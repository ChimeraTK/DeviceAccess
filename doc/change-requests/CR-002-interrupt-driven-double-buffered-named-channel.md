# CR-002: Test interrupt-driven reads on a double-buffered named channel

Synopsis: Add a simple test proving that the well-tested double-buffer
implementation (the `DoubleBufferAccessor`) is used for an interrupt-driven
`wait_for_new_data` read of a double-buffered named channel (a channel slice of
a 2D register): the read must return the freshly finished buffer's channel
data, for both buffers. Verification only; no production code changes.

Status: DONE

## Requirements

Aspect: certify that the interrupt-driven access to a double-buffered named
channel uses the existing double-buffer implementation.

- An interrupt-driven `wait_for_new_data` read of a double-buffered named
  channel (a channel slice of an interrupt-triggered 2D register) must go
  through the tested `DoubleBufferAccessor` and return that channel's elements
  of the buffer the simulated firmware just finished writing.
- Both buffers must be covered: one interrupt finishing buffer 0 and one
  finishing buffer 1, each returning that buffer's channel data. The channel
  data written to buffer 0 and buffer 1 must differ, so an implementation that
  always delivers the same buffer would fail one of the two checks. Testing
  only one buffer, or writing identical channel data to both buffers, would
  not catch such an implementation.
- Use a slice that is not the first channel, so a missing byte-offset fold into
  the addresses would be caught.
- Do not re-verify the double-buffer handshake itself (e.g. the enable register
  staying enabled); that is covered by the double-buffer accessor's own tests.
- The existing polled double-buffer test must remain as it is. A new register
  and a separate interrupt test are added; the existing test is not substituted.

## Specifications

Affected components: only `tests/muxedDataAccessor.jmap` and
`tests/executables_src/testNumericAddressedBackendUnified.cpp`. No production
code changes.

- Add a new 2D register `TEST.DBLASYNC` to `tests/muxedDataAccessor.jmap`, a
  copy of `TEST.DBL` (double-buffered, named channels) but with
  `triggeredByInterrupt` set (new interrupt id). `TEST.DBL` itself stays
  interrupt-free. The parser then gives the new register's named-channel slices
  interrupt access and the inherited double-buffer configuration, so the async
  path routes the slice through the existing `DoubleBufferAccessor`.
- Add a new unified test case on the `/TEST/DBLASYNC.1` slice (not the first
  channel), advertising `wait_for_new_data` and raising the test interrupt
  after each firmware-side buffer finish. The channel values written into each
  buffer carry a buffer-dependent offset, so the two buffer paths have distinct
  expectations. The existing `testDoubleBufferedNamedChannelSlices` and the
  `DoubleBufferedNamedChannelSlice` struct are unchanged.

## Test plan

- A simple check on `TEST.DBLASYNC.1`: drive two buffer finishes (one
  finishing buffer 0, one finishing buffer 1), raise the interrupt after each,
  and assert the returned values equal the freshly finished buffer's channel
  data. The channel values written to each buffer carry a buffer-dependent
  offset on top of the same base sequence, so an implementation that always
  delivers the same buffer cannot satisfy both checks. No assertion on
  handshake internals.
- Full sub-suite run (`ctest`) of the numeric addressed backend unified and
  double-buffering tests to ensure the untouched existing tests still pass.
