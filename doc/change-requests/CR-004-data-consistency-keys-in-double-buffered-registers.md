# CR-004: Tests for data consistency keys in double-buffered registers

Synopsis: Certify with tests that an interrupt-driven `wait_for_new_data` read
of a double-buffered register carries the `VersionNumber` of a
`DataConsistencyRealm`, derived from a data-consistency key register read
together with the data. Two key configurations are covered: a plain key, and a
double-buffered key sharing the data register's control state (correlated
swap). Verification only; the production machinery is already in place.

Status: PLANNED

## Requirements

Aspect: data consistency key for interrupt-driven double-buffered reads.

- An interrupt-driven read of a double-buffered register must be delivered with
  the `VersionNumber` the realm returns for the key value read in the same
  transfer group. The production code (`TriggeredPollDistributor`,
  `DoubleBufferAccessor`) is already in place and unchanged.
- Both key configurations must be proven:
  - a plain, non-double-buffered key register (single 32-bit word), and
  - a double-buffered key register sharing the data register's control
    registers (enable, inactive-buffer id, index), so both accessors resolve
    to one control state in `_doubleBufferMutexMap` and swap together.
- Each interrupt delivers the freshly finished buffer tagged with the version
  of the key read at that interrupt; in the correlated configuration, key and
  data come from the same buffer generation.
- A repeated key keeps the delivered `VersionNumber`; a backwards key keeps the
  last one and marks the data `DataValidity::faulty`.

## Specifications

Affected components: the data-consistency/double-buffering tests and the
re-used `tests/muxedDataAccessor.jmap`, which gains two key registers. No
production code, unless a test uncovers a defect.

- Map: reuse `tests/muxedDataAccessor.jmap`. `TEST.DBLASYNC` (double-buffered,
  interrupt 7) is the data register, `TEST.DOUBLE_BUF.ENA` and
  `TEST.DOUBLE_BUF.INACTIVE_BUF_ID` the control state; the slice
  `TEST/DBLASYNC.1` is already exercised there.
- Add two scalar key registers. Keys must be scalar because the key accessor is
  a `ScalarRegisterAccessor<uint64_t>` and only element (0,0) forms the
  `DataConsistencyKey`. Both are single 32-bit words with
  `triggeredByInterrupt: [7]`:
  - `TEST.KEY`, plain, for the basic case;
  - `TEST.KEYDB`, double-buffered with its own buffers but the same control
    registers and index as `TEST.DBLASYNC`, hence correlated (see
    Requirements).
- In `testDataConsistencyRealm.cpp`, add one test case per configuration, each
  opening the backend with a `DataConsistencyKeys` CDD mapping the respective
  key register to a realm, then driving the firmware side: write key and data
  into the freshly written buffers, set the inactive-buffer id, raise the
  interrupt.
- Each test subscribes with `wait_for_new_data` and drives the two buffers with
  different values, so a wrong buffer or mismatched key/data pair fails. It
  checks the delivered `VersionNumber` against `realm->getVersion(key)`,
  iterates both buffer indices, and covers the repeated/backwards-key cases.
- The existing `testDataConsistencyRealm` tests and map file stay unchanged.

## Test plan

- `TestDataConsistencyKeyDoubleBufferPlain` and
  `TestDataConsistencyKeyDoubleBufferCorrelated` in the data-consistency test
  executable, implementing the two Specifications cases above; the correlated
  case asserts key and data from one buffer generation.
- Full `ctest` of the data-consistency/double-buffering tests plus the three
  executables using `muxedDataAccessor.jmap`
  (`testNumericAddressedBackendUnified`,
  `testNumericAddressedBackendRegisterAccessor`, `testLMapBackendUnified`) to
  confirm the added registers disturb nothing.
