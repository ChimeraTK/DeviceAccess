# CR-010: Integration test for combined muxed double-buffer features

Synopsis: Add one integration test that combines the NumericAddressedBackend
muxed double-buffer features in a single scenario: a double-buffered named
channel in a selectedBy muxed register, driven by an interrupt, with a
data-consistency key and bit ranges in one channel. Each feature must be
provably selected; nothing may be silently dropped. The test exposes a
production bug in the async shared-target accounting for multiple bit-range
children, which this change request fixes.

Depends on: CR-001, CR-002, CR-003, CR-004

Status: PLANNED (from READY TO IMPLEMENT)

## Requirements

Aspect: certify that the NumericAddressedBackend muxed double-buffer feature
combination works together.

- A named-channel slice of a 2D register that is simultaneously
double-buffered, interrupt-triggered, muxed by a register-level `selectedBy`
and contains bit ranges in one channel must deliver the freshly finished
buffer's channel data through the slice on interrupt.
- The test must fail when any one feature is silently dropped:
- both buffers must be delivered with distinct data (double buffering),
- delivery must be gated by the selector (selectedBy),
- the delivered `VersionNumber` must be the realm version of the key read in
the same transfer group (data-consistency key),
- the bit-range child slices must extract the correct fields (bit ranges).
- The exercised named-channel slice must not be the first channel, so a missing
byte-offset fold into the addresses would be caught.
- The production code must implement the documented combination in the async
path: when several bit-range children of the same muxed, double-buffered named
channel are subscribed with `wait_for_new_data`, each child must be delivered
its extracted field from the freshly finished buffer. The test must fail while
this is not the case.
- Aspects already covered by CR-001..CR-004 and the selectedBy feature tests
(double-buffer handshake internals, both key configurations individually,
per-channel validity, write rejection of bit ranges) are not re-tested here.

## Specifications

Affected components: a new jmap fixture, the numeric addressed backend unified
test executable, and the bit-range shared-target accounting in the async
double-buffer path.

- New fixture `tests/selectedByCombined.jmap` (DummyBackend): 2D DMA register
`DAQ.DATA`, 4 elements, pitch 32 bits, register-level `selectedBy` (selector
`MUX.SEL` == 1), `triggeredByInterrupt`, and `doubleBuffering` sharing the
`DB.ENA`/`DB.INACTIVE_BUF_ID` control state. Flat named channels: `0` (plain
full word) and `1` (offset 4, with bit-field `children` `Lo`/`Hi`, 16 bits at
`bitShift` 0/16). Plain scalar key register `DAQ.KEY` (single 32-bit word) on
the same interrupt domain, for the `DataConsistencyKeys` mapping.
- New hand-written integration test case in
`tests/executables_src/testNumericAddressedBackendRegisterAccessor.cpp`, built
with the dummy backend, opening
`(dummy?map=selectedByCombined.jmap&DataConsistencyKeys={"/DAQ.KEY":"CombinedRealm"})`
and reading the realm from `DataConsistencyRealmStore` (add the
`async/DataConsistencyRealmStore.h` include).
- Consumer: the `/DAQ/DATA.1` full-word slice with `wait_for_new_data`. Firmware
side drives the selector, the double-buffer handshake (enable,
inactive-buffer id), full 32-bit buffer words, and the key value.
- The firmware finishes both buffers once, each carrying a distinct base value
set (Lo and Hi halves distinct per element and differing between buffers) and
its own key value. After each interrupt the test checks the delivered data,
the realm version of the just-written key, and the two child slices' extracted
half-words.
- Then the selector is set to unselected (`MUX.SEL` == 2) and a buffer finish is
performed: no data must be delivered. Re-selecting (`MUX.SEL` == 1) and
finishing another buffer must deliver again.
- Fix the shared-target accounting in `BitRangeAccessorDecorator` (shared with
`SubArrayAccessorDecorator`): multiple bit-range children of the same raw DMA
target are read in separate transfer sequences within the same async transfer
group (each child wrapped in its own double-buffer accessor), so the check
`_lock.mutex()->useCount() == _sharedAccessors->instanceCount(_target->getId())`
in `doPostRead`/`doPreRead` never matches and `_target->postRead()` is skipped,
leaving the shared raw word stale. The accounting must ensure `postRead()` runs
exactly once per target per transfer across the separate sequences, so each
child's bit-range extraction sees the freshly finished buffer.
- Existing tests and fixtures stay unchanged; the new jmap is used only by the
new test.

## Test plan

- One integration test case running the full scenario: both buffer finishes,
each asserting delivered data, the realm version of that buffer's key value
and the bit-range extraction; then the unselected-and-reselected gating check.
A `std::cout` line names each scenario step. Each "silently dropped" aspect is
tied to a concrete assertion whose failure proves the aspect was dropped:
- the two buffer finishes deliver distinct value sets, so a dropped
double-buffer handshake would fail a value comparison against the expected
buffer,
- after setting `MUX.SEL` to unselected and finishing a buffer, no data is
delivered (an unexpected delivery fails on a read timeout), so a dropped
selector gating would fail that assertion; reselecting and finishing again
must deliver,
- each delivered `VersionNumber` equals the realm version of the key written
in the same transfer group, so a dropped data-consistency mapping would fail
the version comparison,
- the child slices extract the expected Lo/Hi half-words, so a dropped bit
range would fail the field comparison.
- Full sub-suite `ctest` run of the numeric addressed backend register accessor,
double-buffering and data-consistency tests to confirm the new fixture and
test disturb nothing.

## Alternatives considered

- Unified backend test variants for the combination: rejected. The framework
only asserts that the delivered `VersionNumber` increases
(`UnifiedBackendTest.h`), which a silently dropped data-consistency key would
also satisfy, and it cannot express the unselected gating-off check, so two of
the four "nothing silently dropped" aspects would be missed.
