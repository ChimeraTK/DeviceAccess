# CR-004: Tests for data consistency keys in double-buffered registers

Synopsis: Certify that an interrupt-driven `wait_for_new_data` read of a
double-buffered register is tagged with the `VersionNumber` of a
`DataConsistencyRealm`, derived from a data consistency key register read
together with the data. Two key-register configurations are covered: a plain
(non-double-buffered) key register, and a double-buffered key register
sharing the double-buffer control registers with the data register so the
buffer swaps are correlated. Verification only; the underlying machinery is
already implemented.

Status: PLANNED

## Requirements

Aspect: data consistency key for interrupt-driven double-buffered reads.

- When a double-buffered register is triggered by an interrupt (so it
  advertises `AccessMode::wait_for_new_data`) and the backend's
  `DataConsistencyKeys` configuration maps a register in the same async domain
  to a `DataConsistencyRealm`, each delivered data set must carry the
  `VersionNumber` the realm returns for the key value read together with the
  data. This must be proven in tests; the production code (the
  `TriggeredPollDistributor` reading the key and the data in one transfer
  group, and the `DoubleBufferAccessor` selecting the freshly finished buffer)
  is already in place and unchanged by this change request.
- Two key-register configurations are supported by the production code and
  must be covered in tests:
  - a plain, non-double-buffered key register (single 32-bit word), and
  - a double-buffered key register sharing the double-buffer control registers
    (enable register, inactive-buffer register and index) with the data
    register, so both `DoubleBufferAccessor` instances resolve to one shared
    control state in `_doubleBufferMutexMap` and their buffer swaps are
    correlated.
- After each interrupt, the read must return the buffer the simulated firmware
  has finished writing, tagged with the `VersionNumber` matching the key value
  read at that interrupt. For the correlated configuration, the key value and
  the data must come from the same buffer generation.
- An interrupt which does not change the key value must keep the already
  delivered `VersionNumber`; a key value going backwards must keep the last
  delivered `VersionNumber` and mark the data `DataValidity::faulty`, as the
  `TriggeredPollDistributor` already implements.

## Specifications

Affected components: the double-buffering/data-consistency tests and the
re-used JMAP file `tests/muxedDataAccessor.jmap`, which gains two scalar key
registers. Production code is unchanged unless a test uncovers a defect.

- Reuse `tests/muxedDataAccessor.jmap` instead of adding a new map file. Its
  register `TEST.DBLASYNC` (double-buffered, `triggeredByInterrupt: [7]`) and
  the control registers `TEST.DOUBLE_BUF.ENA` and
  `TEST.DOUBLE_BUF.INACTIVE_BUF_ID` already provide the data register and the
  double-buffer control state; the channel slice `TEST/DBLASYNC.1` is already
  driven through the dummy backend in `testNumericAddressedBackendUnified`.
- Add two scalar key registers (single 32-bit words) to that map only:

  - `TEST.KEY`, a plain register with `triggeredByInterrupt: [7]`, for the
    basic case (non-double-buffered key).
  - `TEST.KEYDB`, a double-buffered register whose `doubleBuffering` references
    the same `TEST.DOUBLE_BUF.ENA`, `TEST.DOUBLE_BUF.INACTIVE_BUF_ID` and
    index 0 as `TEST.DBLASYNC`, so it resolves to the same shared control
    state in the backend's `_doubleBufferMutexMap` and its buffer swap is
    correlated with the data register. It has its own primary and secondary
    buffer addresses, distinct from the data buffers.
- The key registers must be scalar: the key accessor is a
  `ScalarRegisterAccessor<uint64_t>` (`TriggeredPollDistributor.h`), and only
  element (0,0) is used to build the `DataConsistencyKey`. A wider register
  would silently read only its first element, so the key must be a single
  word.
- Extend `tests/executables_src/testDataConsistencyRealm.cpp` with a test case
  per configuration that opens the backend with a CDD whose
  `DataConsistencyKeys` parameter maps `TEST.KEY` (basic) or `TEST.KEYDB`
  (correlated) to a realm, then drives the firmware side through the dummy
  backend: write the key value and the data into the freshly written buffers,
  set the inactive buffer id, and raise the interrupt.
- The tests subscribe to the data register with `wait_for_new_data` and drive
  both buffers with different values, so delivering the wrong buffer or a
  mismatched key/data pair would fail the check. They verify the delivered
  `VersionNumber` equals the realm version of the key value read together with
  the data, iterate over both buffer indices, and cover the repeated-key and
  backward-key cases from the Requirements. For the correlated case, key and
  data swap together on the single inactive-buffer id of the shared control
  state.
- For the correlated case, an inconsistency between the key and the data
  accessor would point at the shared-control handshake in
  `DoubleBufferAccessor` (neither of the two sharing accessors writes the
  enable register during the transfer-group read). If the test exposes such a
  defect, a small fix must be applied in `DoubleBufferAccessor` or
  `NumericAddressedBackend`; no redesign is expected.
- The existing `testDataConsistencyRealm` tests and map file remain unchanged.

## Test plan

- New tests `TestDataConsistencyKeyDoubleBufferPlain` (basic case, key
  `TEST.KEY`) and `TestDataConsistencyKeyDoubleBufferCorrelated` (correlated
  double-buffered key, `TEST.KEYDB`) in the data-consistency test executable:
  for a sequence of buffer finishes, each raising the interrupt with an
  increasing key, assert the returned data is the freshly finished buffer and
  the `VersionNumber` matches `realm->getVersion(key)`. The correlated case
  additionally asserts that the key and the data come from the same buffer
  generation (they swap together on the single inactive-buffer id of the
  shared control state). With a repeated key the `VersionNumber` stays, and
  with a backward key the last `VersionNumber` is kept while the data validity
  is `faulty`.
- Full sub-suite run (`ctest`) of the data-consistency and double-buffering
  tests, plus the three executables that consume `muxedDataAccessor.jmap`
  (`testNumericAddressedBackendUnified`,
  `testNumericAddressedBackendRegisterAccessor`, `testLMapBackendUnified`), to
  confirm the two added key registers disturb no existing test.
