# CR-011: Grouped demultiplexing of muxed 2D channel slices

Synopsis: A read coordinator held inside the low-level transfer element
demultiplexes the multiplexed source buffer in a single pass. It covers all
consumers read through that element with identical raw-conversion parameters and
UserType. It serves every read consumer of a NumericAddressed low-level element:
scalar registers, non-muxed 1D arrays and muxed channel slices alike. The write
path is unchanged.

Status: DONE

## Requirements

Aspect: cache-efficient demultiplexing.

- Several channel slices of the same muxed 2D register are read through one
shared low-level transfer element. The multiplexed source buffer is then
iterated once per group of channels, not once per channel.
- A group consists of all consumers read through the shared element with
identical raw-conversion parameters and identical UserType. The parameters
are:
  - data type, width, fractional bits, signed flag and raw type.
- Raw-mode consumers are grouped by their raw-mode flag and UserType: the key
carries the raw-mode flag, so a raw consumer and a converting consumer with
otherwise identical parameters and UserType never collide.
- This matches the efficiency of the 2D accessor
(`NumericAddressedBackendMuxedRegisterAccessor`), which already demultiplexes
one source pass in which all group members are written.
- The group loop makes exactly one type-erased call per group per read. All
per-sample conversion runs in an inlined typed loop over the group's members,
so there is no per-sample indirect call.

Aspect: single read path.

- Every read consumer of a NumericAddressed low-level transfer element is served
by the coordinator of that element. This holds whether or not the element is
shared with other consumers.
- The accessor's former in-place path is kept for writing only. It is the write
converter loop in its `doPreWriteImpl` plus the strided raw copy in its
`doPreWrite`.
- A consumer whose element is not shared with any other consumer keeps the
current observed semantics: values, version number and data validity.
- The read path is uniform over UserTypes: the coordinator template is
instantiated for every UserType. A void consumer needs no special case (see
Specifications).

Aspect: gating and demultiplexing are coupled.

- A group's demultiplexing and conversion happen in one pass over the
multiplexed source buffer (see above). All consumers of a group must therefore
be processed together.
- That pass must not happen while the register is gated, since a gated register
carries no current sample.
- The two cannot be enforced in separate places. The gate belongs with the group
loop, not with each consumer's own post-read step.
- Gating itself is CR-016. The single-pass group loop established here is what
forces the coupling.

Aspect: thread safety.

- The coordinator assumes the thread-safety preconditions of the low-level
element that holds it:
  - concurrent access is supported only between accessors and `TransferGroup`s
    that are not connected to each other, and is ensured at a higher level;
  - within one `TransferGroup`, and within one accessor, no concurrent access is
    supported.
- Sharing a coordinator between consumers only ever happens inside one
`TransferGroup`. The coordinator therefore needs no thread safety of its own.

Aspect: uniform staging.

- Every consumer is served through a staging buffer, so there is one code path
for all consumers.
- The buffer is allocated once per consumer at registration. It is swapped into
the application buffer on each read: an O(1) swap, with no per-read allocation
and no copy.

Aspect: transparency for higher-level accessors.

- Accessors layered on top of the low-level read consumers are unchanged in their
interface: bit ranges, double buffering and interrupt-driven reads.
- They benefit automatically, since they read through the same low-level element.

Aspect: safeguards.

- Each consumer's user buffer is written only from within that consumer's own
post-read step. The transfer-element rules therefore hold:
  - the application buffer is changed only in `doPostRead`;
  - data and meta data are updated together;
  - an exception leaves the buffers unchanged.
- The delivered version number and data validity per consumer are unchanged.
- The coordinator is read-only. The write path is unaffected (see
Specifications).
- The coordinator is non-copyable and non-movable, like the transfer elements
that always hold it. Copying or moving it is never needed.

## Specifications

Affected components: `include/NumericAddressedLowLevelTransferElement.h`,
`include/NumericAddressedBackendRegisterAccessor.h`,
`src/NumericAddressedBackendRegisterAccessor.cc`, and the new coordinator class
in `include/ReadCoordinator.h`/`src/ReadCoordinator.cc`. `RawConverter.h` is not
changed.

- A new class (`detail::ReadCoordinator`) is held by value as a member of
`NumericAddressedLowLevelTransferElement`.
- It is a plain registry plus the demultiplexing and conversion loop.
- With no consumers registered it is inert, and adds no overhead to the
element's transfer path.
- `NumericAddressedBackendRegisterAccessor` registers each read consumer with the
coordinator of its raw element.
- The accessor's own converter loop and raw copy serve only the write path.
That path keeps the existing converter loop and raw copy, and is otherwise
unchanged.
- The coordinator's `doPostReadImpl` is the only read-side conversion loop.
- It still declares a no-op `doPreWriteImpl`, because the shared
`ConverterLoopHelperImpl` template instantiates both virtual members. Nothing
invokes it.
- ASCII-data registers use a completely separate accessor
(`NumericAddressedBackendASCIIAccessor`). They never reach the coordinator and
register nothing.
- This is distinct from a `UserType` of `std::string` on a numeric register.
That is a normal UserType, handled like any other: the raw converter
converts the numeric value to a string.
- Registering is unconditional for the read path, and assumes no property of a
channel slice (in particular not its read-onlyness). The read path is therefore
independent of the write path, and applies to every read consumer.
- The registration carries no internal reset of its own:
  - the constructor starts from a fresh registration;
  - `replaceTransferElement` resets the previous handle before re-registering.
- A synchronous void register does have a low-level element, and registers with
the coordinator like any other consumer. It cannot be read through the public
API, since `VoidRegisterAccessor::read()` requires `wait_for_new_data`, but the
register accessor is created and the element is read internally, so the
coordinator must stay safe for it. An asynchronous void accessor never uses a
low-level element, since it is served by a variable distributor.
- Registration and factory are therefore written to be safe for void by
construction, without a void branch of their own:
  - the helper is created through the general factory directly, not through
    `callForRawType`; that helper is a no-op for `DataType::Void`, so a void
    register would leave the helper unset and the group's first read would
    crash. The factory's own `callWithConverterParams` resolves the raw type and
    covers the void case.
  - the group loop skips the buffer access for a raw type of `ChimeraTK::Void`
    (see below).
- Each consumer carries:
  - the byte offset of its register within the element's raw buffer, taken from
    its address;
  - the stride (`elementPitchBits / 8`);
  - the sample count;
  - a reference to its typed staging vector;
  - its registration handle.
- The staging-vector reference is resolved lazily when the group loop runs.
The reason is the swap: it exchanges the internal buffers of the two vectors
on every read, so no per-read functor or pointer cast is needed.
- The coordinator does not carry data validity. It is produced by the consumer's
own `doPostRead` from the raw accessor, exactly as before this change. Pairing
it with the payload is left to a later change (CR-016).
- A consumer carries no conversion routine. The typed conversion happens inside
the group's shared loop below, and raw consumers are copied there with the
consumer's stride.
- A raw-mode consumer's UserType is the register's raw data type: an integer of
matching size and signedness. It may be signed as well as unsigned.
- The raw copy is therefore guarded by a nested compile-time gate, first
`if constexpr(std::is_integral_v<UserType>)`, then
`if constexpr(ChimeraTK::isRawType<std::make_unsigned_t<UserType>>)` (the
`raw_type` concept in `SupportedUserTypes.h`).
- `std::make_unsigned_t` is defined only for integral and enum types, so the
unsigned counterpart may be formed only behind the integral guard.
- The non-integral UserTypes (`std::string`, `float`, `double`,
`ChimeraTK::Boolean`, `ChimeraTK::Void`) are excluded by the outer integral
guard, not by forming their unsigned counterpart.
- `isRawType` covers unsigned raw types only, while a raw-mode UserType may be
signed, so the unsigned counterpart is checked.
- This covers every valid raw-mode UserType.
- The memcpy is instantiated only for such types, never for any other UserType
(which would be undefined behaviour). It needs no runtime fallback.
- The registration handle is templated on the UserType
(`detail::ReadCoordinator::Registration<UserType>`), and held as a member of
the accessor.
- It owns a `std::vector<UserType>` staging buffer, sized to the consumer's
element count and allocated once at registration, freed on deregistration.
- The coordinator registry itself stays type-erased, so one element can serve
consumers with different UserTypes.
- The group key (`GroupKey`) consists of the raw-conversion parameters (data
type, width, fractional bits, signed flag, raw type) plus the UserType and the
raw-mode flag.
- The raw-mode flag keeps a raw consumer and a converting consumer with
otherwise identical parameters and UserType in separate groups.
- It mirrors the fields the 2D accessor's file-local `ConverterInfo` groups by,
but is its own type.
- When consumers merge into one element in a TransferGroup
(`replaceTransferElement` with `isMergeable`), the adopting accessor
re-registers its consumer with the merged element. All consumers of the group
then end up on the same element's registry.
- An element can belong to at most one transfer group by construction, so the
registry never mixes consumers of different groups.
- The registry groups the consumers by the group key in a
`std::map<GroupKey, size_t>`, mapping each key to its group's stable,
monotonically increasing `groupId`.
- The `groupId` is assigned once when the group is formed. It is what the
group's `ConverterLoopHelper` receives as `implParameter`, so no helper's
`implParameter` can go stale.
- A group is removed only at teardown, so the registry needs no freed-slot
tracking and no stale-index assert.
- Each consumer's run through the group loop is bounded by its own sample
count, so the group stores no shared iteration bound.
- Each consumer is written from its own offset and stride, independent of the
others.
- The group loop reuses `RawConverter::ConverterLoopHelper` directly, cloning the
2D accessor's pattern.
- Each group owns one `std::unique_ptr<RawConverter::ConverterLoopHelper>`,
created once when the group is formed.
- It is created through the general, void-aware factory
`RawConverter::makeConverterLoopHelper<UserType>(info, 0, groupId, *this)`,
with the register info and the coordinator as the PrePostActionImplementor.
- The factory already handles both `FundamentalType::nodata` (raw type
`ChimeraTK::Void`) and a `UserType` of `ChimeraTK::Void`, so
`RawConverter.h` stays unchanged.
- Error messages keep the register name and the channel index, since the full
`NumericAddressedRegisterInfo` is passed with the consumer registration.
- The coordinator plays the role of the accessor. It implements the templated
`doPostReadImpl<UserType, RawType, SignificantBitsCase, FractionalCase,
isSigned>`, with the group's stable `groupId` as `implParameter`.
- The element runs the coordinator in its `doPostRead` when `hasNewData` is true.
- It needs no once-per-read guard of its own:
  - the base class calls `doPostRead` at most once per read: `preRead` is a no-op
    while a read transaction is in progress, and `postRead` delegates to
    `doPostRead` only while that flag is set (see `include/TransferElement.h`);
  - so the shared raw buffer is processed once, even though the element's
    post-read step is entered once per consumer of the group;
  - the exception path (`hasNewData == false`) runs nothing.
- It makes one virtual `converterLoopHelper->doPostRead()` per group.
- That single type-erased call per group per read dispatches into the
coordinator's templated `doPostReadImpl`.
- This performs the inlined typed loop over all consumers of the group, via
per-consumer offsets, writing each consumer into its staging buffer. One
memory copy of the raw sample plus `converter.toCooked` per sample.
- The loop is skipped for a raw type of `ChimeraTK::Void` (a void register): its
element has no data bytes, and the conversion is a no-op, so the buffer must not
be touched. This mirrors the guard of the accessor's former in-place loop.
- There is no per-sample function-call indirection, exactly matching
`NumericAddressedBackendMuxedRegisterAccessor`.
- Each consumer, in its own `doPostRead` and guarded by its own `hasNewData`,
swaps its staging buffer into `NDRegisterAccessor::buffer_2D[0]`. It sets the
version number and data validity from the raw accessor, exactly as before this
change.
- The swap is why the consumer resolves its staging-vector reference lazily
(see above).
- The coordinator never writes into a user buffer.

## Test plan

All added tests live in `tests/executables_src/testGroupedDemultiplexing.cpp`.
The new test map `tests/groupedDemux.jmap` provides a scalar and a non-muxed 1D
array in addition to the 2D registers. Each bullet below names its test case and
the aspect it covers that no other test covers. Every added test is covered by
exactly one bullet.

- `TestGroupedReadAllChannels`: read all channel slices of a muxed 2D register
in one TransferGroup. The slices are added in shuffled (non-contiguous,
out-of-order) order, so several groups form within the one shared element. This
is the broad correctness check of a full-group pass:
  - the members are neither contiguous nor in memory order;
  - the test forces several conversion groups inside one element: the `MIXED`
    register carries channels 0 and 3 sharing one conversion key (signed,
    width 16), plus a same-element differently-keyed unsigned (channel 1) and a
    width-12 (channel 2) channel;
  - it asserts the values, and the shared version number and data validity
    across the slices.
- `TestRawModeSlices`: the raw-copy path (raw UserType, no conversion) on a
muxed 2D slice. It also covers the read-only property of non-interrupt 2D
slices.
- `TestAdjacentRegistersShareMergedElement`: read slices of two adjacent muxed
2D registers in one TransferGroup. Their channels merge into one transfer
element, and are read from their individual byte offsets within the merged raw
buffer.
- `TestScalarReadThroughCoordinator`: read a single scalar register
synchronously. This covers the ungrouped single-sample consumer (a lone
scalar) through the staging-and-swap path, asserting value, version number and
data validity.
- `TestNonMuxedArrayReadThroughCoordinator`: read a non-muxed 1D array
synchronously. This covers a non-strided consumer with `nSamples > 1` and
stride equal to the element data width, asserting all values.
- `TestGroupedNonMuxedConsumers`: read a scalar and a non-muxed 1D array whose
elements merge into one shared element in one TransferGroup. Both are read
through their individual byte offsets.
- `TestRawNonMuxedArray`: read a non-muxed 1D array in raw mode. This covers the
raw-copy path with stride equal to the element data width (non-strided raw
UserType).
- `TestDataConsistencyKeyDoubleBuffer` (existing, in
`tests/executables_src/testDataConsistencyRealm.cpp`): interrupt-driven
double-buffered single channel slice. No dedicated test is added: that suite
already reads the slice `/TEST/DBLASYNC.1` with `wait_for_new_data` through the
coordinator, and checks the read data and version.

The following points are not separate test cases:

- Void registers get no dedicated test: no public read reaches the coordinator
with a void consumer (see Specifications).
- Void-safety is verified by construction, and by the existing void tests.
- The once-per-read coordinator run is not observable through the public API, and
is verified manually.
- The injected-read-error path (the `hasNewData == false` case) is already
covered by `test_B_6_4` of the unified backend test.
- Existing tests must keep passing:
  - single channel slices, and ordinary scalar/1D registers;
  - the bit-range, double-buffer and interrupt-driven suites (e.g.
    `testTransferGroup`, `testDoubleBufferAccessor`, `testDoubleBuffering`,
    `testAsyncRead`).
- The change must not alter the write path or any observed read result.
- No performance benchmarks: performance is verified manually.

## Alternatives considered

- Writing the demultiplexed data directly into each consumer's user buffer during
the shared element's post-read step: rejected for this change.
  - It modifies the application buffers of other transfer elements outside the
    owning accessor's `doPostRead`.
  - That violates the transfer-element rules: data and meta data must be updated
    together, and stay unchanged on exceptions.
- Keeping the in-place read path for an ungrouped consumer (no staging buffer and
no swap when the element serves a single consumer): rejected.
  - It would add a second read path and a special case, for no measurable
    benefit.
  - The uniform staging path costs one buffer per consumer and an O(1) swap per
    read.
- A per-sample typed conversion routine stored per consumer (e.g. a
`std::function`, or a re-implemented dispatch call per sample): rejected.
  - It costs one indirect call per sample.
  - It duplicates the conversion dispatch already implemented in
    `RawConverter.h`.
  - Reusing `RawConverter::ConverterLoopHelper` type-erases once per group per
    read, and inlines the typed conversion loop.
- Forming the group's helper through
`RawConverter::ConverterLoopHelper::makeConverterLoopHelperFixedRaw<UserType,
RawType>` (the variant for a raw type already known in the calling code):
rejected.
  - It routes through `callWithConverterParamsFixedRaw`, which has no case for
    `FundamentalType::nodata` (raw type `ChimeraTK::Void`) or a `UserType` of
    `ChimeraTK::Void`, so it would leave the helper unset for void.
  - `RawConverter::makeConverterLoopHelper` routes through
    `callWithConverterParams`, which handles both, so `RawConverter.h` stays
    unchanged.
- Registering only some consumers (e.g. only muxed channel slices), and keeping an
in-place accessor read path for the rest (scalars and non-muxed 1D arrays):
rejected.
  - It keeps two read paths plus a branch to select them.
  - One coordinator read path for all consumers is simpler, and is the premise a
    later change (CR-016) builds on.
