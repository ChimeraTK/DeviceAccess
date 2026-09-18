# CR-004: Tests for data consistency keys in double-buffered registers

Synopsis: Certify with tests that an interrupt-driven `wait_for_new_data` read
of a double-buffered register carries the `VersionNumber` of a
`DataConsistencyRealm`, derived from a data-consistency key register read
together with the data. Two key configurations are covered: a plain key, and a
double-buffered key sharing the data register's control state (correlated
swap). Verification only; the production machinery is already in place.

Status: IMPLEMENTED

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
- After the firmware finishes buffer 1 and raises the interrupt, the read must
  return that freshly finished buffer (buffer 1), tagged with the realm
  version of the key value written alongside it. In the correlated
  configuration, the key value read must be the one of the same buffer
  generation as the data.

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
  key register to a realm, then driving the firmware side once: write key and
  data into buffer 1, set the inactive-buffer id to 0 so buffer 1 becomes the
  freshly finished buffer, and raise the interrupt.
- Each test writes a distinguishable value into the buffer to be read (buffer
  1) only; the other buffer keeps whatever it held before (zeros or previous
  test data), so a wrong buffer fails the check. It asserts the delivered
  data equals the freshly written buffer and the `VersionNumber` equals
  `realm->getVersion(key)`; the correlated test additionally asserts the key
  value is the one of the same buffer generation as the data.
- Edge cases of the individual features (repeated/backwards key, both buffer
  indices, switching validity) are covered by the existing data-consistency and
  double-buffering tests and are out of scope here.
- The existing `testDataConsistencyRealm` tests and map file stay unchanged.

## Test plan

- `TestDataConsistencyKeyDoubleBufferPlain` and
  `TestDataConsistencyKeyDoubleBufferCorrelated` in the data-consistency test
  executable, each performing one buffer finish (delivered from buffer 1) as
  described in the Specifications; each fails if either double buffering or
  the data consistency key handling is turned off.
- Full `ctest` of the data-consistency/double-buffering tests plus the three
  executables using `muxedDataAccessor.jmap`
  (`testNumericAddressedBackendUnified`,
  `testNumericAddressedBackendRegisterAccessor`, `testLMapBackendUnified`) to
  confirm the added registers disturb nothing.
