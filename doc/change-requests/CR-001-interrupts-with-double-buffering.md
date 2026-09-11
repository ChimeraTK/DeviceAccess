# CR-001: Support interrupts with double buffering in numeric addressed backends

Synopsis: Enable interrupt-driven `wait_for_new_data` reads on double-buffered
registers and on their BUF0/BUF1 named-channel-slice buffer views in the
numeric addressed backends, and prove the combined behaviour with tests.

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
- The BUF0 and BUF1 named-channel-slice buffer views of a double-buffered
  register must also support `AccessMode::wait_for_new_data` when the parent
  register is interrupt-triggered. Each such slice, on the interrupt, must
  return the current contents of the fixed buffer it names (BUF0 or BUF1).
- Requesting `wait_for_new_data` on a BUF0/BUF1 slice whose parent register is
  not interrupt-triggered must keep failing with a `logic_error` as it does
  today.
- The double-buffer firmware handshake (disable swapping, read the current
  buffer number, read the buffer, re-enable swapping) must be performed intact
  when the read is triggered by an interrupt, preserving the concurrent-read
  safety established for the polled double-buffer accessors.

## Specifications

Affected components: `JsonMapFileParser`, `NumericAddressedBackend`,
`NumericAddressedRegisterCatalogue`, the async `TriggeredPollDistributor`
machinery (unchanged), and the double-buffer and async tests.

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

### BUF0/BUF1 named-channel-slice buffer views (the actual change)

- The `JsonMapFileParser` currently creates the BUF0/BUF1 buffer views as plain
  `READ_ONLY` registers (`JsonMapFileParser.cc` slice-BUF block), so they do
  not advertise `wait_for_new_data`.
- Change the parser so that a BUF0/BUF1 slice is tagged interrupt-triggered
  (same interrupt id as the parent register) when and only when the parent
  double-buffered register is itself interrupt-triggered. The slice keeps its
  `READ_ONLY` access for ordinary synchronous reads; only the interrupt
  association is added so `getSupportedAccessModes()` advertises
  `wait_for_new_data`.
- Each slice points at one fixed buffer (BUF0 or BUF1, byte offset folded into
  the address as already done today). When subscribed via
  `wait_for_new_data`, it flows through the unchanged async machinery and
  `TriggeredPollDistributor` reads that fixed buffer via a plain synchronous
  accessor on each interrupt. No `DoubleBufferAccessor` handshake is used for
  the slices, because a slice names one concrete buffer rather than the
  "current" buffer.
- Slices of a non-interrupt-triggered double-buffered register keep their
  current non-interrupt `READ_ONLY` behaviour.

### Tests

- Add tests (in the backdoor-based firmware-simulation style of
  `AreaType`/`DoubleBufferedNamedChannelSlice0`) covering both the full
  double-buffered register and the BUF0/BUF1 slices with `wait_for_new_data`,
  exercising multiple buffer-swap cycles and verifying the returned data
  matches the simulated firmware buffer contents.
- Keep the existing polled double-buffering and interrupt tests unchanged and
  passing.

## Test plan

- New combined test: async read of a full interrupt+double-buffer register over
  several buffer swaps, verifying each returned value corresponds to the buffer
  the simulated firmware last filled.
- New combined test: async read of the BUF0 and BUF1 named-channel slice views,
  verifying each returns the contents of its named buffer on the parent's
  interrupt.
- New negative test: `wait_for_new_data` on a BUF0/BUF1 slice of a
  non-interrupt-triggered double-buffered register still throws `logic_error`.
- Full sub-suite run (`ctest`) of the double-buffering and async tests to ensure
  no regression.
