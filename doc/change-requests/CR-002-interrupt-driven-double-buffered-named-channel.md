# CR-002: Test interrupt-driven reads on a double-buffered named channel

Synopsis: Add a simple test proving that the well-tested double-buffer
implementation (the `DoubleBufferAccessor`) is actually used for an
interrupt-driven `wait_for_new_data` read of a double-buffered named channel (a
channel slice of a 2D register): the read must return the freshly finished
buffer's channel data, for both buffers. The circuit (async
`TriggeredPollDistributor` plus `DoubleBufferAccessor` plus the parser's
interrupt-triggered named-channel slices) is already in place; this change
request certifies it with a test and adds no production code.

Status: READY TO IMPLEMENT

## Requirements

Aspect: certify that the interrupt-driven access to a double-buffered named
channel uses the existing double-buffer implementation.

- The named channel slice of an interrupt-triggered 2D register advertises
  `wait_for_new_data`, since the whole 2D register (including all its channel
  slices) updates with the same interrupt. This change request requires a check
  that an interrupt-driven read of such a slice goes through the tested
  `DoubleBufferAccessor` (i.e. no similar but broken alternative path exists).
- On each interrupt, the read must return the channel's elements of the buffer
  the simulated firmware has just finished writing (the inactive buffer at the
  time of the read).
- The check must cover **both** buffers: one interrupt finishing buffer 0 and
  one finishing buffer 1, each returning that buffer's channel data. Testing
  only one buffer would not catch an implementation that always delivers the
  same buffer.
- The test uses a single channel slice that is **not the first channel** (e.g.
  `TEST.DBL.1`, whose byte offset is folded into the data and secondary buffer
  addresses). Using the first channel could mask a missing offset fold.
- The test does not re-verify the double-buffer handshake itself (e.g. the
  enable register staying enabled after the read) - that is covered by the
  double-buffer accessor's own tests. Only the delivered buffer matters here.

## Specifications

Affected components: only the map data in `tests/muxedDataAccessor.jmap` and
the unified backend test file `tests/executables_src/testNumericAddressedBackendUnified.cpp`.
`NumericAddressedBackend`, the async `TriggeredPollDistributor` machinery,
`JsonMapFileParser`, and `DoubleBufferAccessor` are unchanged by this change
request.

### How the path already works (no code change)

- Map change: the 2D register `TEST.DBL` in `tests/muxedDataAccessor.jmap` is
  currently double-buffered but not interrupt-triggered; it becomes
  interrupt-triggered (`triggeredByInterrupt`). The parser then gives its
  named-channel slices (`TEST.DBL.0`, `TEST.DBL.1`) `Access::INTERRUPT`, so
  they advertise `wait_for_new_data`, and each slice inherits the parent's
  double-buffer configuration with the channel byte offset folded into the data
  and secondary buffer addresses.
- The slice's `BUF0`/`BUF1` buffer-view registers stay plain read-only views
  (the same rule as CR-001's out-of-scope section: a fixed buffer view must not
  advertise `wait_for_new_data`). They are exactly what `DoubleBufferAccessor`
  reads on the leaf paths.
- The async path routes the slice through `TriggeredPollDistributor`, which
  builds a synchronous accessor with `wait_for_new_data` removed; for the
  double-buffer slice that accessor is a `DoubleBufferAccessor`, so it performs
  the firmware handshake and inactive-buffer selection on each interrupt.

### Test changes

- The existing `DoubleBufferedNamedChannelSlice` struct is updated to reflect
  the new map state (advertise `wait_for_new_data`, raise the test interrupt
  after the firmware-side buffer finish, mirroring `MuxedNodmaAsync`) so the
  existing `testDoubleBufferedNamedChannelSlices` keeps passing.
- A simple focused check on `TEST.DBL.1` (channel 1, not the first channel):
  drive two buffer finishes with distinct channel values, one finishing buffer 0
  and one finishing buffer 1, raise the interrupt after each, and assert the
  returned values equal the freshly finished buffer's channel data.
- No assertion on the enable register state or other handshake internals; the
  handshake is the tested `DoubleBufferAccessor`'s responsibility.

## Test plan

- The focused check above: interrupt-driven reads of `TEST.DBL.1` return the
  freshly finished buffer's channel data, covered for both buffers.
- Full sub-suite run (`ctest`) of the numeric addressed backend unified and
  double-buffering tests to ensure the map change causes no regression.
