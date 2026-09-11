# CR-001: Support interrupts with double buffering in numeric addressed backends

Synopsis: Enable interrupt-driven `wait_for_new_data` reads on double-buffered
registers in the numeric addressed backends, and prove the combined behaviour
with tests.

Status: READY TO IMPLEMENT

## Requirements

Aspect: interrupt-driven access to double-buffered data blocks.

- A register that the map file declares as both interrupt-triggered
  (`triggeredByInterrupt`) and double-buffered (`doubleBuffering`) must support
  `AccessMode::wait_for_new_data`: requesting an accessor with that flag must
  succeed and must return the newly written data buffer when the interrupt
  fires.
- The full double-buffered register must return, on each interrupt, the buffer
  that the firmware has finished writing (the inactive buffer at the time of
  the read), exactly as the polled `DoubleBufferAccessor` does today.
- The double-buffer firmware handshake (disable swapping, read the current
  buffer number, read the buffer, re-enable swapping) must be performed intact
  when the read is triggered by an interrupt, preserving the concurrent-read
  safety established for the polled double-buffer accessors.

## Specifications

Affected components: `NumericAddressedBackend`, the async
`TriggeredPollDistributor` machinery (unchanged), and the double-buffering and
async tests.

### Full double-buffered register (verified against current state)

- The interrupt-triggered double-buffer register already advertises
  `wait_for_new_data` in `getSupportedAccessModes()` because its register
  access is `INTERRUPT`.
- The async path already routes such a register through
  `TriggeredPollDistributor`, which builds a synchronous accessor with
  `wait_for_new_data` removed. For a double-buffer register that accessor is a
  `DoubleBufferAccessor`, so the firmware handshake and inactive-buffer
  selection already run on each interrupt.
- The change request therefore mainly *verifies* this path works correctly
  end-to-end and fixes any gaps discovered during verification (for example
  buffer-selection timing, `numberOfWords`/element-count handling, or
  handshake locking under interrupt-driven polling).

### BUF0/BUF1 buffer views (deliberately not interrupt-capable)

- The `DoubleBufferAccessor` reads the buffer contents through the leaf BUF0/BUF1
  register paths. The `JsonMapFileParser` creates these BUF0/BUF1 views (for the
  full register and for each named channel slice) as plain `READ_ONLY`
  registers, folding the channel byte offset into the buffer addresses.
- These BUF0/BUF1 views must *not* advertise `wait_for_new_data`. Subscribing to
  one fixed buffer would read the buffer that the firmware is currently writing
  on alternating interrupts, returning inconsistent data. The correct
  interrupt-driven double-buffer access is the full register (see the section
  above), which selects the freshly finished buffer on every interrupt. This
  decision explicitly overrides an earlier draft that made the slice views
  interrupt-capable.

### Implementation gaps fixed while verifying the full-register path

- `getSyncRegisterAccessor` now defaults `numberOfWords` to the element count
  for double-buffer registers before constructing the `DoubleBufferAccessor`.
- `DoubleBufferAccessor` only writes the initial "enable buffer swapping" value
  to the firmware when the device is already open, so creating an accessor
  before the device is opened does not touch the hardware.

### Tests

- Add a test (in the backdoor-based firmware-simulation style of
  `AreaType`) for the full double-buffered interrupt register, exercising
  multiple buffer-swap cycles and verifying the returned data matches the
  simulated firmware buffer contents.
- Keep the existing polled double-buffering and interrupt tests unchanged and
  passing.

## Test plan

- New test: async read of a full interrupt+double-buffer register over several
  buffer swaps, verifying each returned value corresponds to the buffer the
  simulated firmware last filled.
- Full sub-suite run (`ctest`) of the double-buffering and async tests to ensure
  no regression.
