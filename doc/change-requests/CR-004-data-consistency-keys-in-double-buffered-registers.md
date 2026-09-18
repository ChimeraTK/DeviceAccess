# CR-004: Data consistency keys in double-buffered registers

Synopsis: Certify that an interrupt-driven `wait_for_new_data` read of a
double-buffered register is tagged with the `VersionNumber` derived from a
plain (non-double-buffered) data consistency key register which is read
together with the data, and record the two extensions (a double-buffered key
register, and polling without an interrupt) as out of scope. Verification
only; the underlying machinery is already implemented.

Status: PLANNED

## Requirements

Aspect: data consistency key for interrupt-driven double-buffered reads.

- When a double-buffered register is triggered by an interrupt (so it
  advertises `AccessMode::wait_for_new_data`) and the backend's
  `DataConsistencyKeys` configuration maps a plain, non-double-buffered
  register in the same async domain to a `DataConsistencyRealm`, each
  delivered data set must carry the `VersionNumber` the realm returns for the
  key value read together with the data. This basic case must be proven in
  tests; the production code (the `TriggeredPollDistributor` reading the key
  and the data in one transfer group, and the `DoubleBufferAccessor`
  selecting the freshly finished buffer) is already in place and unchanged by
  this change request.
- After each interrupt, the read must return the buffer the simulated firmware
  has finished writing, tagged with the `VersionNumber` matching the key value
  read at that interrupt, for both buffers.
- An interrupt which does not change the key value must keep the already
  delivered `VersionNumber`; a key value going backwards must keep the last
  delivered `VersionNumber` and mark the data `DataValidity::faulty`, as the
  `TriggeredPollDistributor` already implements.

Out of scope (assessed separately, not part of this change request):

- A double-buffered data consistency key register, typically sharing the
  double-buffer control registers with the data register so the buffer swap
  is correlated with the data register.
- Reading the data by polling without an interrupt, typically again with a
  double-buffered data consistency key register on its own.

## Specifications

Affected components: only the double-buffering/data-consistency tests and a
new map file. Production code is unchanged unless a test uncovers a defect.

- Add a new JMAP file for the test (e.g. `tests/dblDataConsistencyKey.jmap`)
  providing one interrupt-driven async domain containing:

  - a plain, non-double-buffered key register (a single 32-bit word) with the
    same interrupt id as the data register,
  - a double-buffered data register with `doubleBuffering` configured
    (secondary buffer address, enable register, inactive-buffer register,
    index) and `triggeredByInterrupt` set to that same interrupt id,
  - the two double-buffer control registers (enable and inactive buffer id),
    single 32-bit words each.
- Extend `tests/executables_src/testDataConsistencyRealm.cpp` (or a dedicated
  new test executable) with a test case that opens the backend with a CDD whose
  `DataConsistencyKeys` parameter maps the plain key register to a realm, then
  drives the firmware side through the dummy backend: write data into the
  buffers, set the inactive buffer id, write the key, and raise the interrupt.
- The test subscribes to the data register with `wait_for_new_data` and drives
  both buffers with different values, so delivering the wrong buffer would
  fail the check. It verifies the delivered `VersionNumber` equals the realm
  version of the key read at that interrupt, and covers the repeated-key and
  backward-key cases from the Requirements.
- The existing `testDataConsistencyRealm` tests and map file remain unchanged.

## Test plan

- New test `TestDataConsistencyKeyDoubleBuffer` (or similarly named) in the
  data-consistency test executable: for a sequence of buffer finishes, each
  raising the interrupt with an increasing key, assert the returned data is
  the freshly finished buffer and the `VersionNumber` matches
  `realm->getVersion(key)`; with a repeated key the `VersionNumber` stays, and
  with a backward key the last `VersionNumber` is kept while the data validity
  is `faulty`.
- Full sub-suite run (`ctest`) of the data-consistency and double-buffering
  tests to ensure the untouched existing tests still pass.
