# CR-011: Grouped demultiplexing of muxed 2D channel slices

Synopsis: When several strided channel slices of a muxed 2D register are read
in one TransferGroup, the multiplexed source buffer is currently iterated once
per channel. Demultiplex all channels with identical raw-conversion parameters
and user type in a single pass, coordinated by a demultiplexer held inside the
shared low-level transfer element.

Status: READY TO IMPLEMENT (from TESTS REVIEWED)

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
per-sample conversion runs in an inlined typed loop, so there is no per-sample
indirect call.

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
- Each slice's user buffer is written only from within that slice's own post-read
step, so the transfer-element rules hold: the application buffer is changed only
in `doPostRead`, data and meta data are updated together, and an exception
leaves the buffers unchanged.
- The delivered version number and data validity per slice are unchanged.
- Writing is unaffected: slices of a 2D register are read-only.
- The demultiplexer is non-copyable and non-movable: it is only ever held inside a
low-level transfer element, and transfer elements are themselves non-copyable and
non-movable, so copying or moving it is never needed.

## Specifications

Affected components: `include/NumericAddressedLowLevelTransferElement.h`,
`include/NumericAddressedBackendRegisterAccessor.h`,
`src/NumericAddressedBackendRegisterAccessor.cc`, `include/RawConverter.h`
(a single factory overload, see below) and the new demultiplexer class in
`include/MuxedChannelDemultiplexer.h`/`src/MuxedChannelDemultiplexer.cc`.

- A new class (e.g. `detail::MuxedChannelDemultiplexer`) is held by value as a
member of `NumericAddressedLowLevelTransferElement`. It is a plain registry
plus the demultiplexing loop; with no consumers registered it is inert and
adds no overhead to the element's transfer path.
- `NumericAddressedBackendRegisterAccessor` registers one consumer with the
demultiplexer of its raw element when the register is strided (element pitch
larger than the element data width; the same property that makes raw slices
copy with stride). Scalar and 1D non-strided registers register nothing and
keep the current `ConverterLoopHelper` and raw copy paths unchanged.
- Each consumer carries: the byte offset of the channel within the 2D area
(from its address), the stride (`elementPitchBits / 8`), the number of
samples, a lazily resolved typed staging-buffer pointer (wrapping decorators
may swap buffers on every read), and its registration handle. It carries no
conversion routine; the typed conversion happens inside the group's shared
loop below, and raw slices are handled there by a strided raw copy.

- The registration handle is templated on the user type
(`detail::MuxedChannelDemultiplexer::Registration<UserType>`) and held as a
member of the accessor. It owns a `std::vector<UserType>` staging buffer sized
to the slice's element count, allocated once at registration and freed on
deregistration. The demultiplexer registry itself stays type-erased, so one
element can serve consumers with different user types.
- The group key equals the 2D accessor's `ConverterInfo` (data type, width,
fractional bits, signed flag, raw type) plus the user type.
- When slices merge into one element in a TransferGroup (`replaceTransferElement`
with `isMergeable`), the adopting slice re-registers its consumer with the
merged element, so all channel consumers of the group end up on the same
element's registry. An element can belong to at most one transfer group by
construction, so the registry never mixes consumers of different groups.
- The demultiplexing loop reuses `RawConverter::ConverterLoopHelper` directly,
cloning the 2D accessor's pattern: each group owns one
`std::unique_ptr<RawConverter::ConverterLoopHelper>`, created once when the
group is formed through the fixed-raw factory. The demultiplexer itself plays
the role of the accessor (implementing the templated
`doPostReadImpl<UserType, RawType, SignificantBitsCase, FractionalCase,
isSigned>`), with the group as `implParameter`.
- The element's post-read therefore runs, per group, one virtual
`converterLoopHelper->doPostRead()`. That single type-erased call per group
per read dispatches into the demultiplexer's templated `doPostReadImpl`, which
performs the inlined typed loop (one memory copy of the raw sample plus
`converter.toCooked` per sample) over all channels of the group via
per-channel offsets, writing each slice into its staging buffer. There is no
per-sample function-call indirection, exactly matching
`NumericAddressedBackendMuxedRegisterAccessor`.
- Since the demultiplexer holds only the group key (channel fields plus user
type) and not a full register info, `RawConverter.h` additionally exposes a
fixed-raw factory overload (and the underlying dispatch helper) that takes the
channel info directly instead of a register info plus channel index; the
existing per-register factories keep their signatures. This is the only change
to `RawConverter.h`.
- Writing is unaffected (non-interrupt 2D slices are read-only), so the
demultiplexer's `doPostReadImpl` is the only conversion loop; it still declares
a no-op `doPreWriteImpl`, since the shared `ConverterLoopHelperImpl` template
instantiates both virtual members, but nothing ever invokes it.

- The registry groups the consumers by the group key. Each consumer's
demultiplexing loop runs up to that consumer's own sample count, so the group
stores no shared iteration bound; each slice is written from its own offset
and stride, independent of the other consumers in the group.
- The element runs the demultiplexing in its `doPostRead`, guarded so it
happens at most once per read: a flag is set when new data is read and
cleared when the demultiplexing has run; `hasNewData == false` (the exception
path) never triggers it. The demultiplexer never writes into a user buffer.
- Each slice, in its own `doPostRead` and guarded by its own `hasNewData`,
swaps its staging buffer into `NDRegisterAccessor::buffer_2D[0]` and sets the
version number and data validity together. The cooked-buffer pointer is
resolved lazily because the swap exchanges the internal buffers of the two
vectors, so the staging data pointer changes on every read.
- This keeps the transfer-element rules intact: the application buffer is only
modified from within the owning accessor's `doPostRead`, data and meta data
are updated together, and an exception leaves all buffers unchanged. The only
additional cost is one staging buffer per consumer and an O(1) swap per read
(no per-read allocation or copy).
- `detail::MuxedChannelDemultiplexer` is non-copyable and non-movable: its copy
and move constructors and assignment operators are deleted. It needs no copy or
move support because it is held only by value inside
`NumericAddressedLowLevelTransferElement`, whose base class `TransferElement` is
itself non-copyable and non-movable; low-level transfer elements are therefore
never copied or moved, and each newly constructed element creates its
demultiplexer fresh.

## Test plan

- Read all channel slices of a muxed 2D register in one TransferGroup and
compare each channel's data against the de-multiplexed expectation, including
a channel that is not the first in memory.
- Cover mixed user types and mixed conversion parameters (signedness, width,
fractional bits) so that several groups form within one read.
- Cover a partial subset of the channels (not all) in one group.
- Cover raw-mode slices.
- Verify the demultiplexing runs only once per transfer-group read by checking
that the shared element's demultiplexing flag is set and cleared correctly,
even when multiple slices trigger post-read.
- Cover interrupt-driven double-buffered registers to ensure the demultiplexer
and the staging/swap integrate correctly with the existing buffer-manager and
version-number handling.
- Cover the exception path: a runtime error on one slice of a group must leave
the other slices' buffers unchanged.
- Existing tests for single strided slices, ordinary scalar/1D registers, and
the bit-range, double-buffer and interrupt-driven suites must keep passing.
- Assert per-slice version number and data validity match the shared element.
- Read slices of two adjacent muxed 2D registers in one TransferGroup so their
channels merge into one transfer element and are demultiplexed from their
individual byte offsets within the merged raw buffer.
- No performance benchmarks: performance is verified manually.

## Alternatives considered

- A decorator around the low-level element or the strided accessor to
orchestrate the demultiplexing: rejected. The coordinator is wanted as a
member of the element, and a decorator would add a layer to every accessor.
- Reusing `detail::SharedAccessors` directly: rejected. It is typed around a
single cooked buffer shared by bit-range decorators of one register and does
not model per-channel converter/user-type grouping on a shared raw element; a
small registry specific to the low-level element is simpler.
- Writing the demultiplexed data directly into each slice's user buffer during
the shared element's post-read step: rejected for this change. It modifies the
application buffers of other transfer elements outside the owning accessor's
`doPostRead`, violating the transfer-element rules (data and meta data must be
updated together and stay unchanged on exceptions).
- A per-sample typed conversion routine stored per consumer (e.g. a
`std::function` or a re-implemented dispatch call per sample): rejected. It
costs one indirect call per sample and duplicates the conversion dispatch
already implemented in `RawConverter.h`. Reusing
`RawConverter::ConverterLoopHelper` type-erases once per group per read and
inlines the typed conversion loop.



