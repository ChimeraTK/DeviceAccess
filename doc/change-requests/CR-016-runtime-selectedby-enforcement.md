# CR-016: Runtime enforcement of `gatedBy` in the `NumericAddressedBackend`

Synopsis: The `gatedBy` metadata the parser records is now enforced at runtime.
A register or channel slice with a `gatedBy` reports valid data only while its
gating register holds the configured value. Polled accessors honour the gate.
`wait_for_new_data` subscriptions are not woken while the gate is closed. The
change also unifies the wording on "gate": the jmap key `selectedBy` becomes
`gatedBy`.

Depends on: CR-011

Status: PLANNED

## Requirements

Aspect: runtime enforcement of the `gatedBy` metadata the parser already
provides.

- Terminology:
- The register named in a `gatedBy` is the *gating register*.
- The condition that register forms with the expected value is *the gate*.
- A register, slice or consumer is *gate open* while its gating register
holds the configured value.
- It is *gate closed* otherwise.
- Polled read of a register with a `gatedBy`:
- Whether the physical read happens is unspecified; the implementation may
choose.
- The data is reported as `DataValidity::faulty` while the gate is closed.
- The payload read through a closed gate is unspecified.
- `DataValidity::faulty` is the only guarantee, so consumers must not rely
on the buffer contents.
- 2D register:
- Every channel slice has its own effective gate.
- A closed slice is faulty while the open slices of the same read stay valid.
- A `gatedBy` on the register itself is recorded in the new
`NumericAddressedRegisterInfo::registerGatedBy`.
- That register-level gate gates only the full-register accessor of the
block.
- It is not inherited by the channel slices. The slices stay gated only by
their own per-channel `gatedBy`.
- A register-level `gatedBy` on a 2D register becomes legal input:
- the schema accepts it
- the parser accepts it and records it separately from the per-channel
gates
- `wait_for_new_data` consumer:
- It must not observe a value while its gate is closed.
- Such a value is discarded, so the consumer neither wakes nor sees a new
version number.
- The initial value of a (re)started subscription is exempt:
- it is always delivered
- it is marked `DataValidity::faulty` while the gate is closed
- Gating register access:
- It uses a synchronous scalar accessor (`getRegisterAccessor<uint64_t>`).
- The accessor is requested without `wait_for_new_data`, so it is
synchronous for any correct backend.
- The public entry point is used, so a backend's override stays in play.
- The gating register must be a scalar register (exactly one value).
- Any width and bit alignment is accepted, a bit range included.
- One gate evaluation performs exactly one read of the gating register.
- Gating register values are unsigned concrete integers forming a disjoint set.
The alternatives are therefore mutually exclusive.
- Out of scope:
- Bitmask gates, as a use case. No check forbids a gating register from
being a bitmask: a bit range that happens to be one bit behaves like any
other bit range.
- Gating register chains (a gating register that itself carries a `gatedBy`).
- Relative gating-register paths.

Aspect: unification of the feature wording on "gate".

- The jmap key `selectedBy` becomes `gatedBy`. It keeps its members:
- `register`
- `value`
- The map format version stays `"1.0"`. The feature was only recorded, never
enforced, so no map content carrying the old key is in use.
- The rename covers the file format and the code:
- the JSON schema
- the parser
- the register catalogue
- the Doxygen documentation
- the test fixtures
- the `selectedBy` mention in the `chimeratk-map-to-jmap` tool docstring
(`tools/chimeratk-map-to-jmap:618`)
- `NumericAddressedRegisterInfo::SelectedBy` becomes `GatedBy`. Its members
change accordingly:
- `selectedBy` becomes `gatedBy`
- `val` becomes `uint64_t` (was `int64_t`)
- `registerGatedBy` is a new member. It carries the register-level gate of a
2D register, separate from the per-channel gates.
- The gate value is unsigned everywhere: the jmap `value`, the parser, the
catalogue and the gate accessor.
- The word "selection" is gone. The terms of the runtime aspect above are used
instead. A register carrying a `gatedBy` is described with the adjective
"gated", not with a second state word.
- The `chimeratk-map-to-jmap` tool does not cover this feature, apart from a
stale `selectedBy` mention in its `_build_2d_descriptor` docstring. That
mention is renamed too, so no `selectedBy` survives anywhere.

## Specifications

Affected components:

- `include/NumericAddressedLowLevelTransferElement.h` (header-only):
- read the gate accessors
- enforce the gate where a consumer's raw data is converted
- `include/ReadCoordinator.h` / `src/ReadCoordinator.cc` (from CR-011): the
per-consumer registration also carries:
- the gate
- the consumer's data validity
- its gate result
- `include/NumericAddressedBackendRegisterAccessor.h` /
`src/NumericAddressedBackendRegisterAccessor.cc`:
- build the gate into the registration
- move payload and validity into the accessor buffer
- New `include/GatedByDecorator.h`:
- the decorator the backend puts on the synchronous accessor of a gated
register
- the small `ClosedGateQuery` mixin through which the asynchronous
distributor reads the gate result
- `include/async/TriggeredPollDistributor.h` /
`src/async/TriggeredPollDistributor.cc`: `PolledAsyncVariable` reads the gate
result and suppresses the delivery through a closed gate.
- `include/NumericAddressedBackend.h` / `src/NumericAddressedBackend.cc`: wrap
the synchronous accessor of a gated register in `GatedByDecorator`.
- `include/NumericAddressedBackendMuxedRegisterAccessor.h` /
`src/NumericAddressedBackendMuxedRegisterAccessor.cc`: enforce the
register-level gate of a 2D register on the full-register accessor, which
the `GatedByDecorator` path does not reach.
- `include/NumericAddressedRegisterCatalogue.h` /
`src/NumericAddressedRegisterCatalogue.cc`:
- new `registerGatedBy` member
- the shared `GatedBy` type, renamed from `SelectedBy`, with an unsigned
`val`
- `src/JsonMapFileParser.cc` / `schemas/jmap.schema.json`:
- record and accept the register-level gate of a 2D register
- rename the key and the type
- make the gate `value` unsigned, so a negative `value` is rejected
- reword the diagnostics to the gate wording
- reword the schema description strings that still contain
`selection`/`Selects` (`schemas/jmap.schema.json:326`, `:397`, `:401`), so
no `selection` wording survives
- apply the existing selector checks to `registerGatedBy`: the selector
register must exist, must not itself be gated, and the gate is allowed
only on a read-only register
- `doc/jmapFormat.dox`:
- rename the key
- rename the anchor `jmap_channel_selection`
- rename the anchor `jmap_module_selection_inheritance`
- rename the anchor `jmap_register_selection` and its heading
`Conditional register selection` (`doc/jmapFormat.dox:137`), and update
the `\ref jmap_register_selection` at `doc/jmapFormat.dox:358`, so no
`selection` wording survives
- reword the prose to the gate wording
- document runtime gating
- `tests/...`:
- rename the fixtures and the expectations
- add the new tests (see Test plan)

### Renamed format

- `gatedBy` is used throughout:
- the jmap key
- the schema
- the parser
- the catalogue type
- the documentation
- the fixtures
- `mapFormatVersion` stays `"1.0"`, so no parser version check changes.
- `selectedByInheritance.jmap` is renamed to `gatedByInheritance.jmap` and
rewritten.
- `simpleJsonFile.jmap` and `muxedPolled.jmap` keep their names and are
rewritten.
- The parser test names and expectations follow.
- The rename is otherwise a plain substitution of the wording. The grammar of
`gatedBy` (an object with `register` and `value`) is unchanged.
- The per-channel inheritance rules are unchanged. A register-level gate on a
2D register is no longer inherited by its channels (see "2D register-level
gate").
- The `value` member becomes an unsigned integer (`uint64_t`) in the schema,
the parser and the catalogue.
- `tools/chimeratk-map-to-jmap` does not implement this feature. Its only
reference is a stale `selectedBy` mention in the `_build_2d_descriptor`
docstring (`tools/chimeratk-map-to-jmap:618`), which the rename updates so
no `selectedBy` survives.

### Gate in the low-level element

- The gate is enforced where a consumer's raw data is converted. That is the
shared `NumericAddressedLowLevelTransferElement` and its `ReadCoordinator`,
so the async path shares one enforcement implementation with the polled one.
- The per-consumer `GatedByDecorator` (see "Gate decorator") only carries the
query. It is transparent and not a second enforcement point.
- CR-011 registers every read consumer with the element's
`detail::ReadCoordinator`. The gate is attached to that same registration.
- Next to the staging buffer the consumer carries:
- the gate: a `boost::shared_ptr<NDRegisterAccessor<uint64_t>>` for the
gating register, the type `getRegisterAccessor<uint64_t>` returns
(`src/NumericAddressedBackend.cc:156-158`), plus the expected value
- its `DataValidity`
- the gate result
- The element collects the gate accessors of its consumers in a `TransferGroup`
of its own.
- Gate accessors merge through the existing `replaceTransferElement()`
mechanism when their gating register is the same or adjacent. One gating
register is therefore read once per transfer, however many consumers it
gates.
- The group is add-only:
- the element hands a gate accessor to the group when its registration is
created
- it skips one the group already holds, so the re-registration of a
transfer-group merge adds nothing
- the group shares the ownership of the accessors with the registrations
- a teardown discards the element, its registrations and its group together,
so nothing has to be removed from the group
- `TransferGroup` offers no removal anyway
- `doReadTransferSynchronously()` reads the gate group once, so all hardware
access happens in the transfer phase, not in the post-read step
(`TransferElement.h:498`). The coordinator then reads each consumer's gate
value from its gate accessor.
- The gate group is independent of the `TransferGroup` the caller may have put
the accessors into. Its read is nested in that group's transfer phase.
- The gate belongs with the coordinator's group loop, which demultiplexes and
converts a group in one pass (CR-011, aspect "gating and demultiplexing are
coupled"). Each consumer's gate is evaluated while its group is processed:
- an open-gate consumer converts its payload as usual and keeps
`DataValidity::ok`
- a closed-gate consumer is left unconverted and marked
`DataValidity::faulty`
- The gate result is recorded separately from the validity. A faulty value
alone cannot tell the two reasons apart (see "Gate result").
- A gate is evaluated at most once per read. A change of the gating register
is picked up on the next read.
- Preconditions are checked up front: the gate accessor exists, is readable and
its register is scalar. No `logic_error` is thrown during the transfer.

### Accessor construction

- The gating register's accessor is obtained with
`getRegisterAccessor<uint64_t>(gatingRegisterPath, 0, 0, {})`.
- Without `wait_for_new_data` the accessor is synchronous for any correct
backend, so no subscription is created.
- The public entry point is used so a backend's override stays in play, the
same way `TriggeredPollDistributor` obtains its synchronous accessor
(`include/async/TriggeredPollDistributor.h:73-77`).
- The gating register must be a scalar register. A multi-element or 2D gating
register is rejected when the gated accessor is created.
- No further check is made: any width and bit alignment is accepted, a bit
range included. A bit-range gate is the backend's bit-range accessor for that
register (`src/NumericAddressedBackend.cc:164-191`) and needs no special
handling.
- The gate is built into the registration of:
- a gated scalar/1D accessor
- every gated channel slice of a 2D register
- The full-register accessor of a 2D register with a register-level `gatedBy`
is not a registration consumer. It enforces the gate itself (see "2D
register-level gate").
- CR-011 registers every read consumer of the low-level element, strided or
not. The gate is therefore attached uniformly instead of being special-cased
per conversion path.
- The accessor's `doPostRead()` copies two things into its own buffer:
- the payload, from the staging buffer
- the validity, from its registration
- `DoubleBufferAccessor` is unchanged. It already propagates the inner
buffer's validity (`src/DoubleBufferAccessor.cc:91-94`).
- The accessor exposes its `_readRegistration` through a getter. The gate
result of its consumer can therefore be read from outside.
- `GatedBy` is the metadata type shared by the catalogue and the registration.

### 2D register-level gate

- The parser records a register-level `gatedBy` on a 2D register in
`NumericAddressedRegisterInfo::registerGatedBy`. It is separate from the
per-channel `gatedBy` its channels carry, and it is not inherited by them.
- The per-channel gates are unaffected. They continue to gate the channel
slices.
- The full-register accessor of a 2D register is a
`NumericAddressedBackendMuxedRegisterAccessor`. It is not one of the
registration consumers:
- it has no low-level transfer element (`getInternalElements()` is empty)
- `replaceTransferElement()` is a no-op
- it owns its transfer, so it registers nothing with a
`detail::ReadCoordinator`
- The `GatedByDecorator` and the registration mechanism therefore do not
reach it. It enforces the register-level gate itself:
- it obtains the gating register accessor the same way as the other
accessors (`getRegisterAccessor<uint64_t>(...)`, scalar check, see
"Accessor construction")
- it reads it once in its transfer phase, next to the raw block read
- a closed gate marks the whole block `DataValidity::faulty` in
`doPostRead()` instead of `ok`, so the payload is unspecified
- The channel slices keep using the per-consumer mechanism above.
- `schemas/jmap.schema.json` no longer forbids `gatedBy` on a register with
`channels`.
- `doc/jmapFormat.dox` describes the register-level gate.

### Gate decorator

- `NumericAddressedBackend::getSyncRegisterAccessor()` wraps the accessor of a
gated register in `GatedByDecorator<UserType>`. The synchronous accessor the
distributor polls is therefore the decorator, which is the outermost object.
- The decorator locates the `NumericAddressedBackendRegisterAccessor` in its
target's `getInternalElements()` chain. It keeps a reference to that
accessor's registration and reads the consumer's gate result from it.
- The lookup is a `dynamic_cast` over the chain. It runs once, when the
accessor is created, and is not on any read path.
- The decorator is otherwise transparent. It delegates the read and forwards
the value, the validity and the version number of its target unchanged.
- The decorator brings in `ClosedGateQuery`, a small non-template interface
with the single query `isGateClosed()`. The distributor can ask it without
knowing the `UserType` or the concrete accessor.

### Gate result

- A forwarded value is `DataValidity::faulty` for two unrelated reasons:
- the gate is closed
- the distributor reports a forced-faulty state, a data-consistency version
regression (`TriggeredPollDistributor::getForceFaulty()`)
- Only the gate reason suppresses the delivery. The gate result is therefore
read from the registration, not inferred from the value's validity.
- The gate result is per consumer. A registration belongs to exactly one
consumer, so:
- each slice of a 2D register carries its own gate result
- the full-register accessor of a 2D register with a register-level gate
determines its own result directly, not through a registration
- the result is never shared
- `GatedByDecorator` reads the result through the registration it resolved at
construction. A transfer-group merge re-seats the registration in place when
it replaces the low-level element, so the reference stays valid while the
accessor lives.
- The result reaches the distributor because the decorator is the accessor the
distributor polls. The distributor resolves `ClosedGateQuery` once and reads
the gate result through it.
- The plumbing stays local to `TriggeredPollDistributor` and its internal
`PolledAsyncVariable`. No other part of the asynchronous system knows about
the gate.

### Suppressing the delivery

- `PolledAsyncVariable::send()` skips the send while `isGateClosed()` reports a
closed gate. The consumer therefore neither wakes nor sees a new version
number.
- `AsyncVariableImpl::send()` loses its `final`, so `PolledAsyncVariable` can
override it. The base implementation is otherwise unchanged.
- An open gate is handled by the base implementation, the forced-faulty state
included. A value that is faulty only through the forced-faulty state
carries an open gate and is still delivered.

### Initial value on (re)activation

- A subscription delivers a mandatory initial value after it becomes active,
and again after each recovery. The value is produced without a gate
evaluation (`include/async/AsyncAccessorManager.h:261-264`,
`src/NumericAddressedBackend.cc:246-262`).
- `PolledAsyncVariable` tells the initial value apart by version:
- it remembers the version number of the last value it sent
- it compares that with `DeviceBackend::getVersionOnOpen()`
- The distributor exposes the version on open, since `_backend` is protected
in its base.
- While the remembered version is older than the version on open, the device
has been (re)opened since that delivery. The next value is therefore an
initial value, and it is sent unconditionally, even through a closed gate.
Once a newer value has been sent, the gate result is consulted again.
- The same comparison already detects the first access after opening in
`SubArrayAccessorDecorator` and `BitRangeAccessorDecorator`
(`include/SubArrayAccessorDecorator.h:159`,
`include/BitRangeAccessorDecorator.h:288`).

### No core API change

- No member function is added to `NDRegisterAccessor`, `TransferElement` or
`TransferGroup`. In particular:
- no gate query
- no skip control
- no removal of a transfer-group member
- The gate group uses the existing `TransferGroup` API unchanged:
- `addAccessor()`
- `read()`
- No gate-specific member or behaviour is added to the class.
- The gate result, the `ClosedGateQuery` mixin and the registration getter are
backend-internal:
- the mixin is brought in only by `GatedByDecorator`
- it is read only by `PolledAsyncVariable`
- the getter is on `NumericAddressedBackendRegisterAccessor`

### Layer structure

The diagrams show the objects a register with `gatedBy` hands out. They show
who owns the gate result, and how the result travels to the object that acts
on it. `v` and `|` are the read path going inward, `^` is returned data. The
channel on the right is the registration member carrying the gate result.

The synchronous stack, outermost object first:

```
+------------------------------------------+
| Consumer of the synchronous accessor     |
| - calls read()                           |
+------------------------------------------+
|                       ^
| read()                | value, validity,
|                       | version
v                       |
+------------------------------------------+
| GatedByDecorator<UserType>  (outermost)  |
| - the object the caller receives         |
| - transparent: forwards value,           |
|   validity and version                   |
| - brings in ClosedGateQuery::            |<------+
|   isGateClosed()                         |       |
| - reads the gate result via the          |       | the decorator reads
|   registration, resolved once at         |       | the gate result here
|   construction (dynamic_cast             |       |
|   over getInternalElements())            |       |
+------------------------------------------+       |
|                                        |
| read() (delegated)                     |
|                                        |
v                                        |
+------------------------------------------+       |
| BitRangeAccessorDecorator /              |       |
| SubArrayAccessorDecorator                |       |
| - optional, transparent                  |       |
+------------------------------------------+       |
|                                        |
| read() (delegated)                     |
|                                        |
v                                        ^
+------------------------------------------+       +-------------------------+
| NumericAddressedBackendRegisterAccessor  |       | Registration<UserType>  |
|   <UserType>  (base accessor)            |------>| - base accessor member  |
| - one per consumer                       |       | - one per consumer      |
| - owns the Registration member           |       | - holds the GATE RESULT |
+------------------------------------------+       |   (open / closed)       |
|                                        | - written by the        |
| read()                                 |   coordinator           |
|                                        | - not inferred from     |
v                                        |   the value validity    |
+------------------------------------------+       +-------------------------+
| NumericAddressedLowLevelTransferElement  |       ^
| - shared by every read consumer          |       |
| - owns the gate TransferGroup, read in   |       |
|   the transfer phase                     |       |
+------------------------------------------+       |
|                                        |
| read()                                 |
|                                        |
v                                        | the coordinator writes
+------------------------------------------+       | the GATE RESULT after
| ReadCoordinator                          |       | each read
| - per-consumer Consumer<UserType>        |-------+
| - computes the GATE RESULT               |
+------------------------------------------+

```

The asynchronous case. The synchronous accessor in the middle is the same
stack as above, reached through the object
`NumericAddressedBackend::getSyncRegisterAccessor()` returns:

```
+------------------------------------------+
| Consumer (application)                   |
| - subscribes                             |
| - receives pushed values                 |
+------------------------------------------+
|                       ^
| send()                | subscribe()
|                       |
v                       |
+------------------------------------------+
| PolledAsyncVariable                      |
| - inside TriggeredPollDistributor        |
| - one poll per distribute() call         |
| - resolves ClosedGateQuery once          |
| - overrides send():                      |
|     gate closed + not initial            |
|       -> nothing is pushed               |
|     otherwise AsyncVariableImpl::send()  |
| - initial value by version:              |
|     _lastSentVersion <                   |
|       getVersionOnOpen()                 |
+------------------------------------------+
|                       ^
| poll()/read()         | isGateClosed()
|                       | -> gate result
v                       |
+------------------------------------------+
| GatedByDecorator<UserType>  (outermost)  |
| - the accessor the distributor polls     |<-+
| - brings in ClosedGateQuery              |  |
+------------------------------------------+  | the gate result is read
|                                   | through the registration
| read()                            | the accessor resolved
|                                   |
v                                   |
+------------------------------------------+  |
| identical to the sync case:              |  ^
| - base accessor owns the registration    |--+
| - low-level element (shared)             |
| - ReadCoordinator computes the           |
|   GATE RESULT                            |
+------------------------------------------+




```

## Test plan

- Renamed format:
- the parser uses `gatedBy`
- the schema uses `gatedBy`
- the fixtures `gatedByInheritance.jmap`, `simpleJsonFile.jmap` and
`muxedPolled.jmap` carry the new key
- `mapFormatVersion` stays `"1.0"`
- Parser and catalogue:
- a register-level `gatedBy` on a 2D register is accepted and recorded in
`registerGatedBy`, independent of the per-channel values
- the former inheritance of a register-level gate into the channel slices is
gone: the channels report no gate unless they carry one of their own (this
replaces the expectations for `/REG2D_TOP` in
`testJsonMapFileParser.cpp:1283` and `:1291`)
- the extended selector checks apply to `registerGatedBy`: an undefined
selector register, a gated selector register and a gate on a writable
register are rejected
- a register without `gatedBy` reports no gate (regression)
- Polled scalar/1D (`testNumericAddressedBackendRegisterAccessor.cpp`):
- an open gate returns `DataValidity::ok` and the data
- a closed gate returns `DataValidity::faulty`, with no assertion on the
payload bytes
- changing the gating register returns the other alternative at the same
address
- Gating register:
- the gate is read with `getRegisterAccessor<uint64_t>`; a gating register
that supports `wait_for_new_data` gets no subscription
- a non-scalar (multi-element or 2D) gating register is rejected with a
clear error when the gated accessor is created
- a bit-range gating register is accepted and gates correctly
- an unsigned gate value with the top bit set gates correctly
- the parser rejects a negative `value`
- Polled 2D (`testMultiplexedDataAccesor.cpp`):
- per-channel gating of the slices within one read
- register-level whole-block gate on the full-register accessor, enforced
in `NumericAddressedBackendMuxedRegisterAccessor`
- independence of the two gates on a register that uses both
- Async (`testAsyncRead.cpp`, new fixture `tests/gatedByInterrupt.jmap`):
- open-gate interrupts deliver a new value
- an interrupt through a closed gate does not wake the consumer and
publishes no new version number
- a change of the gating register while the consumer is pending
- the consumer never observes the data of a closed alternative
- Merged gate reads: consumers whose gating register is the same are served by
one group member, so the gating register is accessed once per transfer
(counted on the backend). This also holds after a transfer-group merge of
the payload accessor.
- Regression:
- the double-buffer suite
- the muxed suite
- the async suite
- the parser suite
- registers without `gatedBy` behave unchanged

## Alternatives considered

- Gate enforcement in a decorator on the synchronous accessor, with a shared
`GatedByState` handshake: rejected.
- The gate must be enforced where the raw data is converted.
- That element is shared by consumers with different gates.
- Preparing payload and validity there is one implementation instead of two.
- It removes the shared-state handshake as well.
- The new `GatedByDecorator` only reads the result; it enforces nothing.
- An inner `TransferGroup` in the low-level element owning the gate accessors:
rejected.
- `TransferGroup` has no removal API.
- The element re-registers its consumers on a merge, so members could be
neither moved nor removed.
- The registration owns the gate accessor instead.
- Gate query and transfer-skip API on `NDRegisterAccessor`
(`setSkipOnClosedGate()`, `isGateOpen()`): rejected.
- Only the async path of one backend needs it.
- It widens the core interface of every backend and decorator.
- Skipping the physical read on the async path: rejected.
- `TransferGroup` drives the low-level elements directly and never calls a
decorator's `doReadTransferSynchronously` (spec E.4).
- A low-level element may serve more than one subscription, so a
per-subscription skip cannot be expressed there.
- The read is harmless on a shared address. Only the conversion and the
delivery are suppressed.
- Lockstep `cppext::future_queue<Decision>` parallel to the data transport
queue, consumed in the notification continuation: rejected.
- It duplicates the synchronisation of the transport queue.
- It needs a handshake per value.
- It puts the discard in the notification continuation, instead of the
distributor that already owns the synchronous accessor.
- Querying the shared low-level element for the gate result: rejected.
- The element serves every consumer of an address, so it cannot identify a
consumer's per-accessor registration.
- That registration lives on the base accessor, which
`getInternalElements()` names.
- A mixin on `NumericAddressedBackendRegisterAccessor` that the distributor
casts to directly: rejected.
- Bit-range and sub-array decorators sit above the base accessor and would
hide the mixin, so the chain still had to be walked.
- The decorator that is already outermost carries the mixin instead.
- A dedicated non-template provider interface for the registration: rejected.
- The lookup runs only when the accessor is created.
- Trying the two `isRaw` instantiations of
`NumericAddressedBackendRegisterAccessor` in the `dynamic_cast` is simpler
than adding a base class.
- Signalling a closed gate through an unchanged version number alone:
rejected.
- It cannot prevent the wake.
- It gives the version number a second meaning.
- Telling the gate result apart from the forced-faulty state through the
version number: rejected.
- It gives the version number a second meaning as well.
- It breaks if the `DataConsistencyRealm` ever returns an equal version.
- Recognising the initial value by its delivery position: rejected.
- Initial values recur after every recovery, so the detection must key on
the (re)open.
- `DeviceBackend::getVersionOnOpen()` marks it reliably.
- Rejecting the old `selectedBy` key explicitly: not needed. The feature was
only ever recorded, so no map content carrying the old key is in use. The
format version therefore stays `"1.0"`.
- Gate lookup on the backend-agnostic base catalogue
(`BackendRegisterCatalogueBase::getGatedBy`): not needed. Only the
`NumericAddressedBackend` consults the gate, and it looks the metadata up in
its own catalogue. No backend-agnostic hook is introduced.
- Enforcing the gate in other backends (LMap, PCIe): out of scope. This change
is limited to the `NumericAddressedBackend`.
- Restricting the gating register to a byte-aligned register of at most 64 bit:
rejected. It would need an explicit check that rejects valid use cases (a
bit-range gate, a wider gate), for no benefit.
- Obtaining the gate accessor through the private
`NumericAddressedBackend::getSyncRegisterAccessor()`: rejected.
`getRegisterAccessor()` is synchronous without `wait_for_new_data` and does
not skip a backend's override.

## Deferred issue

- DI-1 [FOLDED-IN] Register-level `gatedBy` on a 2D register: the plan changes the documented behaviour. Today the parser applies a register-level `selectedBy` on a 2D register as the default for all channels (doc/jmapFormat.dox:406, tests/selectedByInheritance.jmap:112, tests/executables_src/testJsonMapFileParser.cpp:1283/1291), but CR-016 says it is 'recorded separately ... independent of the per-channel gates' and gates the whole block on the full-register accessor, while the 'Renamed format'...
- DI-2 [FOLDED-IN] The full-register accessor of a 2D register is a `NumericAddressedBackendMuxedRegisterAccessor` (include/NumericAddressedBackendMuxedRegisterAccessor.h:106 getInternalElements() == {}, :108 replaceTransferElement() no-op, own doReadTransferSynchronously; CR-011 serves only the channel slices). It has no low-level element and no `ReadCoordinator` registration, so the planned `GatedByDecorator`+registration mechanism (and the 'the full-register accessor of a 2D register with a r...
- DI-3 [FOLDED-IN] The Requirements claim `NumericAddressedRegisterInfo::registerSelectedBy` becomes `registerGatedBy`, but no such member exists anywhere in the code (src/JsonMapFileParser.cc stores the register-level gate only in `Channel::selectedBy`/`info.channels.back().selectedBy`). The Specifications correctly call `registerGatedBy` a new member, so the Requirements wording is factually wrong and internally inconsistent.
- DI-4 [FOLDED-IN] The plan does not extend the existing parser validations to the new register-level `registerGatedBy`: the checks that a selector register exists, that it is not itself gated, and that `selectedBy` is used only on read-only registers iterate `reg.channels[].selectedBy` (src/JsonMapFileParser.cc:695-721 and 365-378). Stored separately, a register-level gate would bypass them, accepting an undefined or gated selector register.
- DI-5 [FOLDED-IN] Specifications lists a non-existent affected component `src/NumericAddressedLowLevelTransferElement.cc`; `NumericAddressedLowLevelTransferElement` is header-only (include/NumericAddressedLowLevelTransferElement.h, 212 lines) and no such .cc exists anywhere in the tree (also absent in CR-011, which named only the header).
- DI-6 [FOLDED-IN] Requirements claim the word 'selection' is eliminated, but Specifications state the `chimeratk-map-to-jmap` tool is unaffected, while tools/chimeratk-map-to-jmap:618 (`_build_2d_descriptor` docstring) still names the key `selectedBy`; the two statements conflict and a stale mention survives the rename scope.
- DI-7 [NEW] Rename scope leaves 'selection' survivors, contradicting Requirements ('The word "selection" is gone', CR-016:92) and the plan's own 'no selectedBy survives anywhere' (CR-016:97). The Specifications doc bullet (CR-016:142-146) names only the anchors jmap_channel_selection and jmap_module_selection_inheritance, but doc/jmapFormat.dox also has the heading/anchor 'Conditional register selection' / jmap_register_selection (doc/jmapFormat.dox:137) plus its cross-reference \ref jmap_register_selection (doc/jmapFormat.dox:358), neither of which is in the rename scope.
- DI-8 [NEW] Same 'selection' survivor class in the schema: schemas/jmap.schema.json:326 ('Selects this channel by writing a value to a selection/mux register'), :397 and :401 ('selection/mux register') keep the word 'selection'/'Selects'. The schema bullet (CR-016:134-138) only says 'rename the key and the type' and 'reword the diagnostics'; the description strings are not covered.
- DI-9 [NEW] Gate accessor type is stated inconsistently: 'Gate in the low-level element' (CR-016:189) types the gate as 'std::shared_ptr<ScalarRegisterAccessor<uint64_t>>', but the plan obtains it with getRegisterAccessor<uint64_t> (CR-016:231), which returns boost::shared_ptr<NDRegisterAccessor<uint64_t>> in this codebase (src/NumericAddressedBackend.cc:156-158). ScalarRegisterAccessor is the user-facing abstractor returned by Device::getScalarRegisterAccessor, not by getRegisterAccessor (include/ScalarRegisterAccessor.h:25, include/Device.h:266-270).
- DI-10 [NEW] Minor editorial inconsistency: 'Renamed format' (CR-016:162-165) says the three fixtures selectedByInheritance.jmap, simpleJsonFile.jmap and muxedPolled.jmap are 'renamed and rewritten', but only selectedByInheritance.jmap is actually renamed (to gatedByInheritance.jmap, Test plan CR-016:506); the other two keep their names and are only rewritten.
