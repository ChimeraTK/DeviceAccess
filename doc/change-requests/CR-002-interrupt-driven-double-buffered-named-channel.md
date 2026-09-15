# CR-002: Test interrupt-driven reads on a double-buffered named channel

Synopsis: Add tests proving that an interrupt-driven `wait_for_new_data` read
of a double-buffered named channel (a channel slice of a 2D register) already
works end-to-end in the numeric addressed backends. The circuit (async
`TriggeredPollDistributor` plus `DoubleBufferAccessor` plus the parser's
interrupt-triggered named-channel slices) is already in place; this change
request certifies it with tests and adds no production code.

Status: READY TO IMPLEMENT

## Requirements

Aspect: certify interrupt-driven access to a double-buffered named channel.

- The named channel slice of an interrupt-triggered 2D register already
  advertises `wait_for_new_data`, since the whole 2D register (including all
  its channel slices) updates with the same interrupt. This change request
  requires verification of this already implemented feature in tests.
- On each interrupt, a read of the slice must return that channel's elements of
  the buffer the simulated firmware has just finished writing (the inactive
  buffer at the time of the read), over consecutive buffer swaps, exactly as
  the polled double-buffered named channel slice does today.
- The double-buffer firmware handshake (disable swapping, read the current
  buffer number, read the buffer, re-enable swapping) must be performed intact
  for the interrupt-triggered slice, preserving the concurrent-read safety
  established for the polled double-buffer accessors.

## Specifications

Affected components: only the map data in `tests/muxedDataAccessor.jmap` and
the unified backend test struct `DoubleBufferedNamedChannelSlice` in
`tests/executables_src/testNumericAddressedBackendUnified.cpp`.
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
  and secondary buffer addresses (`src/JsonMapFileParser.cc`, slices of an
  interrupt-driven 2D register).
- The slice's `BUF0`/`BUF1` buffer-view registers stay plain read-only views
  (the same rule as CR-001's out-of-scope section: a fixed buffer view must not
  advertise `wait_for_new_data`). They are exactly what `DoubleBufferAccessor`
  reads on the leaf paths.
- The async path routes the slice through `TriggeredPollDistributor`, which
  builds a synchronous accessor with `wait_for_new_data` removed; for the
  double-buffer slice that accessor is a `DoubleBufferAccessor`, so the
  firmware handshake and inactive-buffer selection already run on each
  interrupt.
- Test change only: extend `DoubleBufferedNamedChannelSlice` to advertise
  `wait_for_new_data` and to raise the test interrupt after the firmware-side
  buffer finish (mirroring `MuxedNodmaAsync`), so the `UnifiedBackendTest`
  async battery verifies the returned values.

## Test plan

- Interrupt-driven reads of the `/TEST/DBL.1` slice over several buffer swaps,
  verifying each returned value set corresponds to the channel slice of the
  buffer the simulated firmware last finished (extend the existing
  `testDoubleBufferedNamedChannelSlices` unified test case).
- Full sub-suite run (`ctest`) of the numeric addressed backend unified and
  double-buffering tests to ensure no regression.
