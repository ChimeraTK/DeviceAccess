# CR-002: Test interrupt-driven reads on a double-buffered named channel

Synopsis: Add a simple test proving that the well-tested double-buffer
implementation (the `DoubleBufferAccessor`) is used for an interrupt-driven
`wait_for_new_data` read of a double-buffered named channel (a channel slice of
a 2D register): the read must return the freshly finished buffer's channel
data, for both buffers. Verification only; no production code changes.

Status: READY TO IMPLEMENT

## Requirements

Aspect: certify that the interrupt-driven access to a double-buffered named
channel uses the existing double-buffer implementation.

- An interrupt-driven `wait_for_new_data` read of a double-buffered named
  channel (a channel slice of an interrupt-triggered 2D register) must go
  through the tested `DoubleBufferAccessor` and return that channel's elements
  of the buffer the simulated firmware just finished writing.
- Both buffers must be covered: one interrupt finishing buffer 0 and one
  finishing buffer 1, each returning that buffer's channel data. Testing only
  one buffer would not catch an implementation that always delivers the same
  buffer.
- Use the `TEST.DBL.1` slice (not the first channel, so a missing byte-offset
  fold into the addresses would be caught).
- Do not re-verify the double-buffer handshake itself (e.g. the enable register
  staying enabled); that is covered by the double-buffer accessor's own tests.

## Specifications

Affected components: only `tests/muxedDataAccessor.jmap` and
`tests/executables_src/testNumericAddressedBackendUnified.cpp`. No production
code changes.

- Make the 2D register `TEST.DBL` in `tests/muxedDataAccessor.jmap`
  interrupt-triggered. The parser then gives its named-channel slices interrupt
  access and the inherited double-buffer configuration, so the async path routes
  the slice through the existing `DoubleBufferAccessor`.
- Update the `DoubleBufferedNamedChannelSlice` struct so the existing
  `testDoubleBufferedNamedChannelSlices` still passes with the new map state.

## Test plan

- A simple check on `TEST.DBL.1`: drive two buffer finishes with distinct
  channel values (one finishing buffer 0, one finishing buffer 1), raise the
  interrupt after each, and assert the returned values equal the freshly
  finished buffer's channel data. No assertion on handshake internals.
- Full sub-suite run (`ctest`) of the numeric addressed backend unified and
  double-buffering tests to ensure the map change causes no regression.
