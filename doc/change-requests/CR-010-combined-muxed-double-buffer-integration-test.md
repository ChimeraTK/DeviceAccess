# CR-010: Integration test for combined muxed double-buffer features

Synopsis: Verification-only change request. Add one integration test that
combines the NumericAddressedBackend muxed double-buffer features in a single
scenario: a double-buffered named channel in a selectedBy muxed register, driven
by an interrupt, with a data-consistency key and bit ranges in one channel.
Each feature must be provably selected; nothing may be silently dropped.

Depends on: CR-001, CR-002, CR-003, CR-004

Status: IN PROGRESS (from READY TO IMPLEMENT)

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
- Verification only: no production-code change. Aspects already covered by
CR-001..CR-004 and the selectedBy feature tests (double-buffer handshake
internals, both key configurations individually, per-channel validity, write
rejection of bit ranges) are not re-tested here.

## Specifications

Affected components: a new jmap fixture and the numeric addressed backend unified
test executable. No production code.

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

## Deferred issue

- Genuine production-code bug exposed by the CR-010 verification test: the two bit-range child slices Lo and Hi of a double-buffered, selectedBy, interrupt-driven named channel do not deliver correct data through the async (wait_for_new_data) path; they report new data but return 0 instead of the expected 16-bit half-words.
- Root cause: in the async path each subscribed accessor (including each bit-range child, each wrapped in its own DoubleBufferAccessor) is placed into the TriggeredPollDistributor's internal TransferGroup (src/async/TriggeredPollDistributor.cc:122). Two children (…/Lo/BUF0 and …/Hi/BUF0) share the same raw DMA target; the shared-target accounting in BitRangeAccessorDecorator::doPostRead (include/BitRangeAccessorDecorator.h:214) requires _lock.mutex()->useCount() == _sharedAccessors->instanceCount(target). Because the double-buffer wrapper reads Lo.BUF0 and Hi.BUF0 in separate transfer sequences within the group, useCount never reaches instanceCount (2), so _target->postRead() is skipped and the shared buffer holds stale/zero data.
- Empirically confirmed: with only ONE bit-range child (lo) the whole test passes (data, realm version, both buffer finishes, selector gating, and bit-range extraction are all correct); adding the second child (hi) breaks both lo and hi. So the failure is specific to two bit-range children sharing the parent-channel raw word in the async double-buffer path.
- A manual TransferGroup is not a workaround: TransferGroup::addAccessor rejects wait_for_new_data accessors (src/TransferGroup.cc:197-200), and the CR requires wait_for_new_data.
- Conflict with the CR contract: CR-003 documents that bit-range child slices inherit wait_for_new_data and the BUF0/BUF1 double-buffer views (CR-003 line 30-31), which is exactly the combination CR-010 is meant to verify. The current production code fails this documented combination, so CR-010 (verification-only, no production change allowed) cannot legitimately pass as specified.
- Deliverable kept faithful to the CR spec: tests/selectedByCombined.jmap (fixture, pitch 8 bytes/minimal valid) and a clean test case TestCombinedMuxedDoubleBuffer in tests/executables_src/testNumericAddressedBackendRegisterAccessor.cpp (data+lo+hi) are in the working tree; all debug instrumentation removed; existing tests in the same suite pass (all 24 reported failures are in the new test only). No existing test or fixture was modified. CR document header untouched; nothing committed/pushed.
