# CR-016: Runtime enforcement of `selectedBy` in the `NumericAddressedBackend`

Synopsis: The jmap parser already records `selectedBy` metadata for registers
and channels, but nothing uses it at runtime. This change enforces it: a gated
register or channel slice reports valid data only while its selector register
holds the configured value, for polled accessors and for `wait_for_new_data`
subscriptions, which are not woken while unselected.

Status: IN PROGRESS

## Requirements

Aspect: runtime enforcement of the `selectedBy` metadata the parser already
provides.

- A polled read of a gated register always performs the physical read; the gate
  is evaluated afterwards. Unselected data is reported as
  `DataValidity::faulty`.
- The payload of an unselected value is unspecified. `DataValidity::faulty` is
  the only guarantee; consumers must not rely on the buffer contents.
- Every channel slice of a 2D register is gated by its own effective
  `selectedBy`. Inactive slices are faulty while the selected slices of the
  same read stay valid.
- A `selectedBy` declared on a 2D register gates the whole 2D block on the
  full-register accessor, independently of the per-channel gates of its slices.
- A register-level `selectedBy` on a 2D register becomes legal input: the schema
  and the parser accept it and record it separately from the per-channel
  selections.
- A `wait_for_new_data` consumer must not observe a value while its selection is
  not met. The value is discarded, so the consumer neither wakes nor sees a new
  version number.
- The selector is read through a synchronous scalar accessor
  (`getSyncRegisterAccessor<int64_t>`). It must be byte-aligned and at most 64
  bit wide; one gate evaluation performs exactly one selector read.
- Selector values are concrete integers forming a disjoint set, so the
  alternatives are mutually exclusive. Bitmask selection is out of scope.
- Selector chains (a selector register that is itself gated) and relative
  selector paths are out of scope.

## Specifications

Affected components:

- New `include/SelectedByDecorator.h` / `src/SelectedByDecorator.cc`: gate
  decorator for synchronous accessors.
- New `include/DiscardUnselectedDecorator.h` /
  `src/DiscardUnselectedDecorator.cc`: asynchronous decorator that discards
  unselected deliveries.
- `include/NumericAddressedBackend.h` / `src/NumericAddressedBackend.cc`: build
  the gate decorators and wrap the async accessor of a gated register.
- `include/NumericAddressedRegisterCatalogue.h` /
  `src/NumericAddressedRegisterCatalogue.cc`: new `registerSelectedBy` member
  and the shared `SelectedBy` type.
- `src/JsonMapFileParser.cc` and `schemas/jmap.schema.json`: record and accept
  the register-level selection of a 2D register.
- `doc/jmapFormat.dox`: document runtime gating.
- `tests/...`: new tests and fixtures (see Test plan).

### Gate decorator

- `SelectedByDecorator<UserType>` is an `NDRegisterAccessorDecorator<UserType>`
  on the gated accessor. It owns a `ScalarRegisterAccessor<int64_t>` for the
  selector register and the expected value.
- It never skips a transfer: the physical read is always delegated, so the gate
  can be evaluated after the read and a later selection change is picked up.
- `doPostRead()` evaluates the gate. Selected: the read is delegated unchanged.
  Unselected: the application buffer is marked `DataValidity::faulty` and the
  payload of the wrapped accessor is left untouched, so the inactive layout is
  not converted into the user buffer.
- The decorator is the only place that evaluates the selector, in both the
  polled and the async path.

### Accessor construction

- `NumericAddressedBackend` wraps a `SelectedByDecorator` around the accessor of
  a gated register: a scalar/1D accessor, every gated channel slice of a 2D
  register, a double-buffered accessor, and the full-register accessor of a 2D
  register carrying a register-level `selectedBy`.
- `DoubleBufferAccessor` itself is unchanged.
- `SelectedBy` is the metadata type shared by the catalogue and the decorator.

### 2D register-level selection

- The parser records a register-level `selectedBy` on a 2D register in
  `NumericAddressedRegisterInfo::registerSelectedBy`, separate from the
  per-channel `selectedBy` its channels carry.
- The per-channel selections are unaffected and continue to gate the channel
  slices.
- `schemas/jmap.schema.json` no longer forbids `selectedBy` on a register with
  `channels`; `doc/jmapFormat.dox` describes the register-level gate.

### Async discard decorator

- `NumericAddressedBackend` wraps the async accessor it hands out for a gated
  `wait_for_new_data` register in `DiscardUnselectedDecorator<UserType>`.
- The decorator inserts itself into the notification continuation of the
  accessor's `_readQueue`, which is the void continuation of the data transport
  queue (`TransferElement.h:863-866`).
- When a value marked `DataValidity::faulty` is forwarded, it throws
  `ChimeraTK::detail::DiscardValueException`. Per spec 8.2.2 this has the same
  effect as if no entry had been pushed: the consumer does not wake and observes
  neither a new value nor a new version number.
- Otherwise the decorator is transparent: it delegates the read and forwards
  validity and version number unchanged.
- No other component is affected. `TriggeredPollDistributor`,
  `AsyncNDRegisterAccessor` and `AsyncAccessorManager` keep their current
  behaviour and need no knowledge of the gate.

### Validity contract

- The discard decorator detects a gated delivery through the `DataValidity` of
  the delivered value.
- Within the `NumericAddressedBackend` this is a closed contract: `faulty` on a
  gated value is produced only by the gate decorator, and only while the
  register is unselected.
- The forced-faulty state is the one exception (see DI-1).

### No core API change

- No member function is added to `NDRegisterAccessor` or `TransferElement`, in
  particular no selection query and no skip control.

## Test plan

- Parser and catalogue: a register-level `selectedBy` on a 2D register is
  accepted and recorded in `registerSelectedBy`, independent of the per-channel
  values; a register without `selectedBy` reports no selection (regression).
- Polled scalar/1D (`testNumericAddressedBackendRegisterAccessor.cpp`): the
  selector matching returns `DataValidity::ok` and the data; the selector
  differing returns `DataValidity::faulty` without assertion on the payload
  bytes; changing the selector returns the other alternative at the same
  address.
- Polled 2D (`testMultiplexedDataAccesor.cpp`): per-channel gating of the slices
  within one read; register-level whole-block gate on the full-register
  accessor; independence of the two gates on a register that uses both.
- Async (`testAsyncRead.cpp`, new fixture `tests/selectedByInterrupt.jmap`):
  selected interrupts deliver a new value; an interrupt while unselected does
  not wake the consumer and publishes no new version number; a selector change
  while the consumer is pending; the consumer never observes the inactive
  alternative's data.
- Regression: the double-buffer, muxed, async and parser suites; registers
  without `selectedBy` behave unchanged.

## Alternatives considered

- Gate query and transfer-skip API on `NDRegisterAccessor`
  (`setSkipOnUnselected()`, `isSelected()`): rejected. Only the async path of
  one backend needs it, and it widens the core interface of every backend and
  decorator while the gate state is already observable in the delivered value's
  validity.
- Skipping the physical read on the async path: rejected. `TransferGroup` drives
  the low-level elements directly and never calls a decorator's
  `doReadTransferSynchronously` (spec E.4), and a low-level element may serve
  more than one subscription, so a per-subscription skip cannot be expressed
  there. The read is harmless on a shared address; only the conversion and the
  delivery are suppressed.
- Suppressing the delivery in `fillSendBuffer()`: rejected. It requires the
  distributor to query the gate of the subscription and to special-case the
  initial value, and it duplicates what the void-queue continuation of spec
  8.2.2 provides for exactly this purpose.
- Signalling "unselected" through an unchanged version number alone: rejected.
  It cannot prevent the wake and gives the version number a second meaning.
- Gate lookup on the backend-agnostic base catalogue
  (`BackendRegisterCatalogueBase::getSelectedBy`): not needed. Only the
  `NumericAddressedBackend` consults the gate and it looks the metadata up in
  its own catalogue, so no backend-agnostic hook is introduced.
- Enforcing selection in other backends (LMap, PCIe): out of scope; this change
  is limited to the `NumericAddressedBackend`.

## Deferred issue

- DI-1 [NEW] Forced faulty: the delivered value is also marked faulty when the
  distributor reports a forced faulty state, for example an exception of the
  trigger or of the subscription. The discard decorator must not swallow such a
  value. How the gate-faulty and the forced-faulty causes are told apart is to
  be clarified in a second step.
- DI-2 [NEW] Initial value of a subscription activated while unselected: the
  discard decorator would discard it as well. Whether such an initial value must
  still be delivered, and how it is told apart from a later unselected value, is
  to be clarified in a second step.
