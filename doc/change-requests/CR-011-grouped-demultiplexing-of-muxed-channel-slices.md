# CR-011: Grouped demultiplexing of muxed 2D channel slices

Synopsis: When several strided channel slices of a muxed 2D register are read
in one TransferGroup, the multiplexed source buffer is currently iterated once
per channel. Demultiplex all channels with identical raw-conversion parameters
and user type in a single pass, coordinated by a demultiplexer held inside the
shared low-level transfer element.

Status: PLANNED (from DONE)

## Requirements

Aspect: cache-efficient demultiplexing.

- When several strided channel slices of the same muxed 2D register are read
  through one shared low-level transfer element, the multiplexed source buffer
  is iterated once per group of channels, not once per channel.
- A group consists of all channels read through the shared element with
  identical raw-conversion parameters (data type, width, fractional bits,
  signed flag, raw type) and identical user type; raw-mode slices are grouped
  by user type, since they are copied without conversion.
- This matches the efficiency of the 2D accessor
  (`NumericAddressedBackendMuxedRegisterAccessor`), which already demultiplexes
  one source pass per group.
- The demultiplexing makes exactly one type-erased call per group per read; all
  per-sample conversion runs in an inlined typed loop, so there is no
  per-sample indirect call.

Aspect: activation.

- Every strided channel slice of a muxed 2D register is read through the
  demultiplexer of its low-level transfer element, whether or not that element
  is shared with other slices.
- A slice whose element is not shared keeps the current observed semantics
  (values, version number, data validity).

Aspect: transparency for higher-level accessors.

- Accessors layered on top of the strided slices (bit ranges, double
  buffering, interrupt-driven reads) are unchanged in their interface and
  benefit automatically, since they read through the same low-level element.

Aspect: safeguards.

- The demultiplexing runs at most once per transfer-group read and only for
  freshly read data, although the element's post-read step is entered once per
  consumer slice of the group.
- Each slice's user buffer is written only from within that slice's own
  post-read step, so the transfer-element rules hold: the application buffer is
  changed only in `doPostRead`, data and meta data are updated together, and an
  exception leaves the buffers unchanged.
- The delivered version number and data validity per slice are unchanged.
- Writing is unaffected: slices of a 2D register are read-only.
- The demultiplexer is non-copyable and non-movable: it is only ever held
  inside a low-level transfer element, and transfer elements are themselves
  non-copyable and non-movable, so copying or moving it is never needed.

## Specifications

Affected components: `include/NumericAddressedLowLevelTransferElement.h`,
`include/NumericAddressedBackendRegisterAccessor.h`,
`src/NumericAddressedBackendRegisterAccessor.cc`, `include/RawConverter.h`
(see below) and the new demultiplexer class in
`include/MuxedChannelDemultiplexer.h`/`src/MuxedChannelDemultiplexer.cc`.

- A new class (e.g. `detail::MuxedChannelDemultiplexer`) is held by value as a
  member of `NumericAddressedLowLevelTransferElement`. It is a plain registry
  plus the demultiplexing loop; with no consumers registered it is inert and
  adds no overhead to the element's transfer path.
- The member function bodies in `include/MuxedChannelDemultiplexer.h`
  (`Consumer<UserType>` constructor, `Group<UserType>::consumersEmpty()`,
  `Group<UserType>::rawCopy()`, `Registration<UserType>::~Registration`) are
  defined at the end of the header, ordered by declaration, including for
  template member functions.
- `NumericAddressedBackendRegisterAccessor` registers one consumer with the
  demultiplexer of its raw element when the register is strided (element pitch
  larger than the element data width; the same property that makes raw slices
  copy with stride). Scalar and 1D non-strided registers register nothing and
  keep the current `ConverterLoopHelper` and raw copy paths unchanged.
- `registerWithDemultiplexer()` is called only for strided slices and carries
  no internal reset or `_isStrided` guard of its own: the constructor starts
  from a fresh registration, `replaceTransferElement` resets the previous
  handle before re-registering, and the callers decide whether the slice is
  strided (so the guard lives at the call sites).
- Each consumer carries: the byte offset of the channel within the 2D area
  (from its address), the stride (`elementPitchBits / 8`), the number of
  samples, a reference to the consumer's typed staging vector, resolved lazily
  at demultiplexing time because the swap exchanges the internal buffers of the
  two vectors on every read (no per-read functor or pointer-cast indirection),
  and its registration handle. It carries no conversion routine; the typed
  conversion happens inside the group's shared loop below, and raw slices are
  handled there by a strided raw copy. A raw-mode slice's user type is the
  register's raw data type, an integer of matching size and signedness, so it
  may be signed as well as unsigned; the strided raw copy is therefore guarded
  by the compile-time gate that `UserType` is trivially copyable
  (`std::is_trivially_copyable_v<UserType>`, which covers every valid raw-mode
  user type and excludes e.g. `std::string`); the strided memcpy is
  instantiated only for such types, never for any other user type (which would
  be undefined behaviour), and needs no runtime fallback.

- The registration handle is templated on the user type
  (`detail::MuxedChannelDemultiplexer::Registration<UserType>`) and held as a
  member of the accessor. It owns a `std::vector<UserType>` staging buffer
  sized to the slice's element count, allocated once at registration and freed
  on deregistration. The demultiplexer registry itself stays type-erased, so
  one element can serve consumers with different user types.
- The group key equals the 2D accessor's `ConverterInfo` (data type, width,
  fractional bits, signed flag, raw type) plus the user type.
- When slices merge into one element in a TransferGroup
  (`replaceTransferElement` with `isMergeable`), the adopting slice
  re-registers its consumer with the merged element, so all channel consumers
  of the group end up on the same element's registry. An element can belong to
  at most one transfer group by construction, so the registry never mixes
  consumers of different groups.
- The demultiplexing loop reuses `RawConverter::ConverterLoopHelper` directly,
  cloning the 2D accessor's pattern: each group owns one
  `std::unique_ptr<RawConverter::ConverterLoopHelper>`, created once when the
  group is formed through the fixed-raw factory. The demultiplexer itself plays
  the role of the accessor (implementing the templated
  `doPostReadImpl<UserType, RawType, SignificantBitsCase, FractionalCase,
  isSigned>`), with the group's vector index as `implParameter`.
- The element's post-read therefore runs, per group, one virtual
  `converterLoopHelper->doPostRead()`. That single type-erased call per group
  per read dispatches into the demultiplexer's templated `doPostReadImpl`,
  which performs the inlined typed loop (one memory copy of the raw sample plus
  `converter.toCooked` per sample) over all channels of the group via
  per-channel offsets, writing each slice into its staging buffer. There is no
  per-sample function-call indirection, exactly matching
  `NumericAddressedBackendMuxedRegisterAccessor`.
- `RawConverter` error messages for the demultiplexer path keep the register
  name and the channel index. The upper layer
  (`NumericAddressedBackendRegisterAccessor`) therefore passes a full
  `NumericAddressedRegisterInfo` (which already includes the register name)
  along with the consumer registration, so a group forms its conversion loop
  helper through the existing per-register fixed-raw factory (register info
  plus channel index) and error messages stay informative. A new factory
  overload taking only the channel info is kept only if that is overall simpler
  or the code structure clearly benefits; the existing per-register factories
  keep their signatures. This is the only possible change to `RawConverter.h`.
- Writing is unaffected (non-interrupt 2D slices are read-only), so the
  demultiplexer's `doPostReadImpl` is the only conversion loop; it still
  declares a no-op `doPreWriteImpl`, since the shared
  `ConverterLoopHelperImpl` template instantiates both virtual members, but
  nothing ever invokes it.

- The registry groups the consumers by the group key. The groups are stored in
  a `std::vector<Group>`, with a `std::map<GroupKey, size_t>` resolving each
  key to its vector index. The group's vector index is what the group's
  `ConverterLoopHelper` receives as `implParameter`, so the group is identified
  by a plain stable index (no pointer/address-based identity). The per-group
  helper is owned by its group, so it lives exactly as long as the group. A
  group is erased only when it loses its last consumer, which happens only as
  its lone consumer's replacement re-registers (transfer-group merges are
  one-way). Erasure removes the key from the map and frees the group's vector
  slot, recording the index for reuse by a later registration, so it never
  shifts the index of any other live group and no `ConverterLoopHelper`'s
  `implParameter` index goes stale; an `assert` guards this no-erasure-while-
  reused assumption by checking that no other key in the registry still
  resolves to the freed slot. Each consumer's demultiplexing loop runs up to
  that consumer's own sample count, so the group stores no shared iteration
  bound; each slice is written from its own offset and stride, independent of
  the other consumers in the group.
- The element runs the demultiplexing in its `doPostRead`, guarded so it
  happens at most once per read. The demultiplexer exposes its state through
  two methods named to express 'the demultiplexing for the current read has not
  run yet and runs once per operation on the first call':
  `demultiplexingPending()` (set from the element's read-transfer step when
  fresh raw data is available) and `pendingDemultiplexing()` (true until the
  demultiplexing has run once). The names set the demultiplexer guard apart
  from the element's own `hasNewData` (true on every consumer-slice invocation
  of the post-read within one read), so `doPostRead` needs no comment
  explaining why both guards are needed. `hasNewData == false` (the exception
  path) never triggers it. The demultiplexer never writes into a user buffer.
- Each slice, in its own `doPostRead` and guarded by its own `hasNewData`,
  swaps its staging buffer into `NDRegisterAccessor::buffer_2D[0]` and sets the
  version number and data validity together (the swap is why the consumer
  resolves its staging-vector reference lazily, see above).
- This keeps the transfer-element rules intact: the application buffer is only
  modified from within the owning accessor's `doPostRead`, data and meta data
  are updated together, and an exception leaves all buffers unchanged. The only
  additional cost is one staging buffer per consumer and an O(1) swap per read
  (no per-read allocation or copy).
- `detail::MuxedChannelDemultiplexer` is non-copyable and non-movable: its copy
  and move constructors and assignment operators are deleted. It needs no copy
  or move support because it is held only by value inside
  `NumericAddressedLowLevelTransferElement`, whose base class `TransferElement`
  is itself non-copyable and non-movable; low-level transfer elements are
  therefore never copied or moved, and each newly constructed element creates
  its demultiplexer fresh.

## Test plan

All added tests live in `tests/executables_src/testGroupedDemultiplexing.cpp`.
Each bullet names its test case and gives, as a sub-bullet, the aspect it
covers that no other test covers; every added test is covered by exactly one
bullet.

- Read all channel slices of a muxed 2D register in one TransferGroup, added in
  shuffled (non-contiguous, out-of-order) order, with several groups forming
  within the one shared element: `TestGroupedReadAllChannels`.
   - The broad correctness check of a full-group pass; the members are neither
     contiguous nor in memory order. The shuffled registration also covers what
     the removed partial-subset test did.
   - The only test that forces several conversion groups inside one element: the
     `MIXED` register carries channels 0 and 3 sharing one conversion key
     (signed, width 16), plus a same-element differently-keyed unsigned
     (channel 1) and a width-12 (channel 2) channel, with value assertions. It
     also asserts the shared version number and data validity across the slices,
     covering what the removed mixed-type and version/validity tests did.
- Cover raw-mode slices: `TestRawModeSlices`.
   - The only test that exercises the strided raw-copy path (raw user type, no
     conversion), including the read-only property of non-interrupt 2D slices.
- Read slices of two adjacent muxed 2D registers in one TransferGroup so their
  channels merge into one transfer element and are demultiplexed from their
  individual byte offsets within the merged raw buffer:
  `TestAdjacentRegistersShareMergedElement`.
   - The only test that verifies transfer-element merging of slices from two
     different registers and per-slice offsets within the larger merged buffer.
- Interrupt-driven double-buffered single strided slice: existing
  `TestDataConsistencyKeyDoubleBuffer` in
  `tests/executables_src/testDataConsistencyRealm.cpp`.
   - No dedicated test is added: that suite already reads the strided
     `/TEST/DBLASYNC.1` with `wait_for_new_data` through the demultiplexer and
     checks the demultiplexed data and version, so the redundant
     `TestInterruptDrivenDoubleBufferSlice` is dropped.
- The once-per-read guard is not observable through the public API and is
  verified manually; the injected-read-error path is already covered by
  `test_B_6_4` of the unified backend test.
- Existing tests for single strided slices, ordinary scalar/1D registers, and
  the bit-range, double-buffer and interrupt-driven suites (e.g.
  `testTransferGroup`, `testDoubleBufferAccessor`, `testDoubleBuffering`,
  `testAsyncRead`) must keep passing.
- Regression guard: the change must not alter the non-strided and
  non-grouped paths.
- No performance benchmarks: performance is verified manually.

## Alternatives considered

- Writing the demultiplexed data directly into each slice's user buffer during
  the shared element's post-read step: rejected for this change. It modifies
  the application buffers of other transfer elements outside the owning
  accessor's `doPostRead`, violating the transfer-element rules (data and meta
  data must be updated together and stay unchanged on exceptions).
- A per-sample typed conversion routine stored per consumer (e.g. a
  `std::function` or a re-implemented dispatch call per sample): rejected. It
  costs one indirect call per sample and duplicates the conversion dispatch
  already implemented in `RawConverter.h`. Reusing
  `RawConverter::ConverterLoopHelper` type-erases once per group per read and
  inlines the typed conversion loop.

## Deferred issue

- The stable-slot machinery of the demultiplexer registry (`_freeSlots`, the
  freed-slot reuse branch, the null-hole representation, the hole-skip guard in
  `run()` and the stale-index `assert` in `reset()`) guards against a scenario
  that cannot occur, and should be removed. A group is genuinely removed only
  at teardown; the registry only ever grows or vanishes wholesale, never loses
  one group while keeping others, so no `ConverterLoopHelper`'s index can go
  stale. Reopen this change request and simplify the registry: give each group
  a stable non-positional identity (monotonically increasing `groupId`) instead
  of the `_groups` vector index, drop the stable-slot parts listed above, and
  keep the per-read cost at one type-erased call per group.
