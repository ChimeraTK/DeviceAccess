# CR-004: Data consistency keys in double-buffered registers

Synopsis: Certify that an interrupt-driven `wait_for_new_data` read of a
double-buffered register is tagged with the `VersionNumber` of a
`DataConsistencyRealm`, derived from a data consistency key register read
together with the data. Two key-register configurations are covered: a plain
(non-double-buffered) key register, and a double-buffered key register
sharing the double-buffer control registers with the data register so the
buffer swaps are correlated. Reading by polling without an interrupt is
recorded as out of scope. Verification only; the underlying machinery is
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

Out of scope (assessed separately, not part of this change request):

- Reading the data by polling without an interrupt, typically with a
  double-buffered data consistency key register on its own. This is the only
  remaining extension and is considerably larger: it needs a new triggering
  mechanism in the async framework, so it is not part of this change request.

## Specifications

Affected components: only the double-buffering/data-consistency tests and a
new map file. Production code is unchanged unless a test uncovers a defect.

- Add a new JMAP file for the test (e.g. `tests/dblDataConsistencyKey.jmap`)
  providing two interrupt-driven async domains:

  - Domain A (basic case): a plain, non-double-buffered key register (a single
    32-bit word) and a double-buffered data register, both with the same
    interrupt id. The data register has `doubleBuffering` configured
    (secondary buffer address, enable register, inactive-buffer register,
    index) and `triggeredByInterrupt` set to that interrupt id.
  - Domain B (correlated case): a double-buffered key register and a
    double-buffered data register with their own buffer addresses, but both
    referencing the same enable register and inactive-buffer register (and
    the same index), so both resolve to one shared control state in the
    backend's `_doubleBufferMutexMap` and swap together. Each carries
    `triggeredByInterrupt` for its own interrupt id.
  - Shared by both domains: the two double-buffer control registers (enable
    and inactive buffer id), single 32-bit words each.
- Extend `tests/executables_src/testDataConsistencyRealm.cpp` (or a dedicated
  new test executable) with a test case per domain that opens the backend with
  a CDD whose `DataConsistencyKeys` parameter maps the respective key register
  to a realm, then drives the firmware side through the dummy backend: write
  the key value and the data into the buffers the firmware will expose, set
  the inactive buffer id so key and data swap together to the freshly written
  buffers, and raise the interrupt.
- The tests subscribe to the data register with `wait_for_new_data` and drive
  both buffers with different values, so delivering the wrong buffer or a
  mismatched key/data pair would fail the check. They verify the delivered
  `VersionNumber` equals the realm version of the key value read together with
  the data, iterate over both buffer indices, and cover the repeated-key and
  backward-key cases from the Requirements.
- For the correlated case, an inconsistency between the key and the data
  accessor would point at the shared-control handshake in
  `DoubleBufferAccessor` (neither of the two sharing accessors writes the
  enable register during the transfer-group read). If the test exposes such a
  defect, a small fix must be applied in `DoubleBufferAccessor` or
  `NumericAddressedBackend`; no redesign is expected.
- The existing `testDataConsistencyRealm` tests and map file remain unchanged.

## Test plan

- New tests `TestDataConsistencyKeyDoubleBufferPlain` (basic case) and
  `TestDataConsistencyKeyDoubleBufferCorrelated` (correlated double-buffered
  key) in the data-consistency test executable: for a sequence of buffer
  finishes, each raising the interrupt with an increasing key, assert the
  returned data is the freshly finished buffer and the `VersionNumber` matches
  `realm->getVersion(key)`. The correlated case additionally asserts that the
  key and the data come from the same buffer generation (they swap together on
  a single inactive-buffer id). With a repeated key the `VersionNumber` stays,
  and with a backward key the last `VersionNumber` is kept while the data
  validity is `faulty`.
- Full sub-suite run (`ctest`) of the data-consistency and double-buffering
  tests to ensure the untouched existing tests still pass.
