# CR-003: Support bit ranges in named channels of a 2D register

Synopsis: Allow the named channels of a 2D register (numeric addressed
backends, JMAP 2D registers with a flat `channels` entry together with
`numberOfElements` and `pitch`) to be bit ranges. A channel may declare a
`children` dictionary of bit fields, each becoming a read-only bit-range
slice, and a channel may itself be a single bit range via its
`representation.bitShift`. The bit-range slices read the strided 2D sample
data correctly.

Status: TESTED

## Requirements

Aspect: bit ranges inside the named channels of a 2D register.

- A named channel of a 2D register may declare a `children` dictionary of bit
  fields (keyed by child name, each value a `representation` with
  `bitShift`/`width`/`type`), exactly like the top-level `children` mechanism
  of 1D registers. Today the parser silently ignores these channel children;
  it must honour them instead.
- A child that is not a proper bit range of the channel's sample word (it
  spans the whole word, or `bitShift` + `width` exceeds `bytesPerElement * 8`)
  is ignored: no slice is created for it, and parsing continues with the
  supported children and slices. This keeps forward compatibility with newer
  map files that use unsupported child features.
- Every bit-field child creates its own read-only channel slice
  `<channel>/<child>`, behaving like the other named-channel slices:
  one-dimensional, `nElements` samples, element pitch as stride, the channel
  byte offset folded into the address, and interrupt access (including
  `wait_for_new_data` and the `BUF0`/`BUF1` buffer views) inherited from the
  parent 2D register. All bit-range slices are read-only; writing to them is
  not supported.
- The parent channel keeps exposing the full sample word, both through the
  whole 2D register and through its own slice `<channel>`. The parent slice is
  unchanged from today: byte-aligned (bit offset 0 within the folded word),
  unshifted, not a bit range. Only the child slices are bit ranges.
- A channel may instead be a single bit range itself by setting `bitShift`
  and `width` in its own `representation`. Its slice `<channel>` is then a
  read-only bit range over every sample word, extracted at the channel
  position, and reads the strided data correctly.
- Reading the whole 2D register keeps its current constraints for such
  channels: obtaining the full-2D register accessor raises a clear error, at
  accessor-obtaining time and not already when parsing, instead of returning
  wrong values.
- A channel or child slice whose path collides with an existing register or
  construct (e.g. a channel named `BUF0` on a double-buffered register) makes
  parsing fail with a `ChimeraTK::logic_error` naming the conflicting path;
  such a slice is never silently skipped.
- A strided named-channel slice (element pitch larger than its element data
  width) must keep working as a strided slice with its full element data
  width and must never be reported as a bit range.

## Specifications

Affected components:

- `include/JsonMapFileParser.h` / `src/JsonMapFileParser.cc`: honour `children`
  on `ChannelTab::Channel`, create the child bit-range slices, and make the
  final bit-range re-scan strided-aware.
- `include/NumericAddress.h` and the numeric-address handling in
  `src/NumericAddressedRegisterCatalogue.cc` (function `getBackendRegister`):
  extend `numeric_address::BAR()` to accept an explicit element pitch (stride).
- `src/NumericAddressedBackend.cc` (function `getSyncRegisterAccessor`): build
  the bit-range target path with the new stride syntax.
- `include/SupportedUserTypes.h`: add `DataType::getNumberOfBytes()` for the
  element data width lookup.
- `BitRangeAccessorDecorator` is intentionally unchanged (see "alternatives
  considered").
- `doc/jmapFormat.dox`: document the new channel bit-range syntax (see
  "Documentation" below).

Parser changes (`JsonMapFileParser`):

- `ChannelTab::Channel` gains a `children` member of the same shape as the
  top-level children (`std::map<std::string, ChannelChild>`), parsed with the
  member's `offset`/`bytesPerElement` as the word context.
- In the `addInfos` named-channel slice loop, after the parent channel slice,
  create one bit-range slice per child at `slicePath/<child>` with:
  `ChannelInfo{bitShift, type, width, fractionalBits, isSigned, rawType =
  int<bytesPerElement*8>}` from the child `representation`, `isBitRange`
  always true, and the parent's `bar`, folded address, `nElements`, element
  pitch, `sliceAccessType`, double-buffer inheritance and `BUF0`/`BUF1`
  creation; a muxed channel's `selectedBy` condition is inherited. The child's
  `engineeringUnit`/`description` are taken from the child. A child slice
  whose path already exists throws a `ChimeraTK::logic_error` naming the
  conflicting path.
- Validate each child before creating its slice: `bitShift` + `width` must
  fit within `bytesPerElement * 8`, and a child spanning the whole word
  (`bitShift` 0, `width` = `bytesPerElement * 8`) is not a bit range. A
  failing child is ignored, for forward compatibility; the supported children
  and slices are still created.
- The automatic bit-range re-scan compares against the element data width
  (from the single channel's raw type via `DataType::getNumberOfBytes()`) and
  only considers registers whose element pitch equals the element data width,
  so strided channel slices are never misclassified even when they share an
  address with another register or their element width is smaller than the
  pitch.
- Factor the `ChannelInfo`-from-`representation` conversion and the
  slice-plus-`BUF0`/`BUF1` creation into helpers shared by the parent channel
  slice and the child bit-range slice, so each exists once. The child keeps
  using the parent slice's channel-shifted secondary buffer address as `BUF1`.
- Replace the parent-slice collision guard
  (`if(catalogue.hasRegister(slicePath)) continue;`) by a
  `ChimeraTK::logic_error` naming the conflicting slice path.
- Migrate the double-buffered `DBLASYNC` register in
  `tests/muxedDataAccessor.jmap` from the legacy `channelTabs` form to the
  flat `channels` form (with `numberOfElements` and `pitch`), like its
  synchronous counterpart `DBL`; the parser creates slices and `BUF0`/`BUF1`
  views only for flat `channels`.

Numeric address stride extension:

- Extend `BAR()/<bar>/<component>` with an optional element pitch component
  `p<pitchBits>` (a multiple of 8) after the optional `u`/`s` width marker:
  `BAR()/<bar>/<address>*<nBytes>u<bitWidth>p<pitchBits>`.
- Semantics: `nElements = nBytes * 8 / pitchBits`, `elementPitchBits =
  pitchBits`, width/raw type from `bitWidth` (8, 16, 32 or 64). Without
  `p<pitchBits>` the behaviour is unchanged (`elementPitchBits = bitWidth`).
  Validate the extra `p` component (pitch and total span must give an
  integral, non-zero `nElements`).

Element data width lookup (`include/SupportedUserTypes.h`):

- Add `DataType::getNumberOfBytes()`, a `size_t` switch over
  `DataType::TheType`: 1, 2, 4 or 8 for the signed/unsigned integers by
  width, 4 for `float32`, 8 for `float64`, `sizeof(ChimeraTK::Boolean)` for
  `Boolean`, 0 for `none`/`Void`, and `max()` of `size_t` for `string`
  (variable length; documented in the Doxygen comment).
- Replace the two places deriving the element data width from the channel raw
  type (the parser's bit-range re-scan and `getSyncRegisterAccessor`'s width
  lookup) with `getNumberOfBytes()`, removing the `callForRawType()` +
  `sizeof` construction and its `std::bad_cast` handling.

Bit-range target construction (`getSyncRegisterAccessor`):

- Replace the target path `BAR()/<bar>/<address>*<size>u<elementPitchBits>`
  with `BAR()/<bar>/<address>*<size>u<width>p<elementPitchBits>`
  (`size = nParentElements * elementPitchBits / 8`); `width` is the slice's
  sample-word data width via `getNumberOfBytes()`, not a hardcoded `u64`. The
  resolved register then has `nElements = nParentElements`, the slice pitch as
  stride and a raw type matching the word; the accessor reads strided and
  `BitRangeAccessorDecorator` extracts the range unchanged. The 1D case is
  unchanged (`pitchBits == width`).

Full-2D register accessor safety:

- `NumericAddressedBackendMuxedRegisterAccessor` keeps its byte-alignment
  requirement and additionally detects registers whose channels are bit ranges
  (e.g. a byte-aligned direct `bitShift`, which today silently yields wrong
  values), raising a `logic_error` pointing at the named-channel slice when
  the 2D accessor is obtained (not while parsing).

Documentation (`doc/jmapFormat.dox`):

- In the 2D multiplexed register section (`\subsection jmap_2d`): document
  that a channel may declare a `children` dictionary of bit fields and may
  itself be a bit range via its `representation` `bitShift`/`width`. Each
  channel creates a read-only slice `<channel>` (the parent word: byte-aligned,
  unshifted, not a bit range) and, for a channel with `children`, one
  read-only bit-range slice `<channel>/<child>` per child; all slices are
  read-only (writes unsupported). Children that are not proper bit ranges
  (spanning the whole word, or extending past it) are ignored. A bit-range
  channel is read through its slice; the whole 2D register read is not
  supported. Cross-reference `\subsection jmap_bitfields`.
- In the bit-fields section (`\subsection jmap_bitfields`): note the same
  `children` mechanism is also available per channel of a 2D register (see
  `\subsection jmap_2d`).

Documentation of the numeric address syntax:

- In `doc/numeric_addresses.dox` and the `BAR()` Doxygen comment in
  `include/NumericAddress.h`: document the full
  `BAR()/<bar>/<address>*<nBytes>u<bitWidth>p<pitchBits>` syntax. Every part
  after `<address>` is optional: `*<nBytes>` the total span in bytes (default
  the element size), the width marker `u<bitWidth>` (unsigned) or
  `s<bitWidth>` (signed) with 8, 16, 32 or 64 (default signed 32-bit), and
  `p<pitchBits>` the element pitch in bits (a multiple of 8) making the
  register strided with `nElements = nBytes * 8 / pitchBits`.

## Test plan

- Parser tests (`testJsonMapFileParser.cpp`): a channel with `children` yields
  the parent word slice (not a bit range) plus one bit-range child slice per
  child with the correct bit offset, width, raw type and `isBitRange`; a
  direct-`bitShift` channel yields a bit-range slice; the strided re-scan does
  not misclassify slices; interrupt and double-buffer inheritance (including
  `BUF0`/`BUF1`) works for child slices; a child slice of a muxed channel
  inherits the channel's `selectedBy` condition (fixture `selectedByCases.jmap`
  `BITFIELD/FD/Ch0`, which combines `children` with `selectedBy`). Extend
  `simpleJsonFile.jmap` so the existing `status`, `VectorSum_I`, `VectorSum_Q`
  channel children are covered.
- Parser tests: a child spanning the whole word or extending past it is
  ignored (parsing succeeds, no slice, valid children and slices still
  created). A path collision throws a `ChimeraTK::logic_error` naming the
  path; inject collisions by modifying an existing jmap (e.g.
  `simpleJsonFile.jmap`) with `nlohmann::json`, writing it to a temporary file
  with a unique name and parsing that file. The dedicated
  `bitRangeBuf0Collision.jmap` and `bitRangeChildBuf0Collision.jmap` files
  are therefore removed.
- Numeric-address tests: the `p<pitchBits>` syntax parses to the expected
  `nElements`/`elementPitchBits` and rejects invalid combinations; the
  existing short forms keep working.
- Backend tests (`testNumericAddressedBackendUnified.cpp`): read a 2D register
  with a bit-field child channel and a direct-`bitShift` channel from the
  dummy backend with known interleaved sample data; the full-word slice and
  every bit-range slice return the expected values. Writing to a bit-range
  slice is rejected. Reading a full 2D register containing a direct bit-range
  channel raises the clear error.
- All new `BOOST_AUTO_TEST_CASE` names start with an upper case letter; in
  particular `testBitRangeNamedChannelSlices` is renamed (e.g.
  `TestBitRangeNamedChannelSlices`).
- Unit tests (`testRawDataTypeInfo.cpp`): `getNumberOfBytes()` returns the
  byte size for every `DataType` value, with `max()` of `size_t` for `string`,
  `sizeof(ChimeraTK::Boolean)` for `Boolean` and 0 for `none`/`Void`.
- Full sub-suite `ctest` run of the numeric addressed backend, parser and
  double-buffering tests to ensure untouched existing tests still pass.

## Alternatives considered

- Dedicated strided bit-range accessor replacing `BitRangeAccessorDecorator`
  for channel slices: rejected, it duplicates the decorator's shared-buffer
  and conversion machinery, whereas the numeric-address stride extension keeps
  the decorator and the whole read path intact and is reusable in
  `numeric_address::BAR()`.
- Extending the full-2D read (`NumericAddressedBackendMuxedRegisterAccessor`)
  to return bit-range values: rejected, bit ranges are exposed through the
  named-channel slices only, keeping the 2D path and its read-modify-write
  constraints unchanged.
- `callForRawType()` + `sizeof` for the element data width lookup: rejected,
  it is verbose, duplicated at both call sites and throws `std::bad_cast` for
  unsupported raw types; the dedicated `getNumberOfBytes()` expresses the
  intent directly and covers every `DataType::TheType` value.

