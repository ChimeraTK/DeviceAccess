# CR-001: Interrupt-driven reads on double-buffered registers (verification)

Synopsis: Add tests proving that interrupt-driven `wait_for_new_data` reads on
the full double-buffered register already work end-to-end in the numeric
addressed backends. The circuit (async `TriggeredPollDistributor` plus
`DoubleBufferAccessor`) is already in place; this change request certifies it
with tests and does not add implementation code.

Status: DONE

## Requirements

Aspect: certify interrupt-driven access to the full double-buffered data block.

- The full double-buffered register already supports `AccessMode::wait_for_new_data`
  when the map file declares it interrupt-triggered (`triggeredByInterrupt`).
  This change request requires verification of this already implemented feature
  in tests.
- On each interrupt, the read must return the buffer that the firmware has
  finished writing (the inactive buffer at the time of the read), exactly as
  the polled `DoubleBufferAccessor` does today.
- The double-buffer firmware handshake (disable swapping, read the current
  buffer number, read the buffer, re-enable swapping) must be performed intact
  when the read is triggered by an interrupt, preserving the concurrent-read
  safety established for the polled double-buffer accessors.

## Specifications

Affected components: only the double-buffering and async tests and their map
files. `NumericAddressedBackend`, the async `TriggeredPollDistributor`
machinery, `JsonMapFileParser`, and `DoubleBufferAccessor` are unchanged by
this change request.

### How the path already works (no code change)

- The interrupt-triggered double-buffer register advertises `wait_for_new_data`
  in `getSupportedAccessModes()` because its register access is `INTERRUPT`.
- The async path routes such a register through `TriggeredPollDistributor`,
  which builds a synchronous accessor with `wait_for_new_data` removed. For a
  double-buffer register that accessor is a `DoubleBufferAccessor`, so the
  firmware handshake and inactive-buffer selection already run on each
  interrupt.
- The `numberOfWords` element-count handling and the "device already open"
  guard in `DoubleBufferAccessor` are provided by the separately merged
  `fix/feat: double buffered named channels` work; they are dependencies of
  this change request and are not part of it.

### Out of scope: BUF0/BUF1 buffer views

- A fixed BUF0/BUF1 buffer view must *not* advertise `wait_for_new_data`.
  Subscribing to one fixed buffer would read the buffer that the firmware is
  currently writing on alternating interrupts, returning inconsistent data. The
  correct interrupt-driven double-buffer access is the full register, which
  selects the freshly finished buffer on every interrupt.

## Test plan

- New test: async read of a full interrupt+double-buffer register, over several
  buffer swaps, verifying each returned value corresponds to the buffer the
  simulated firmware last filled.
- Full sub-suite run (`ctest`) of the double-buffering and async tests to ensure
  no regression.

