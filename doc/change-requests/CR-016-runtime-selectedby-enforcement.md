# CR-016: Runtime enforcement of `gatedBy` in the `NumericAddressedBackend`

Synopsis: The `gatedBy` metadata the parser records is now enforced at runtime.
A register or channel slice with a `gatedBy` reports valid data only while its
gating register holds the configured value. Polled accessors honour the gate.
`wait_for_new_data` subscriptions are not woken while the gate is closed. The
change also unifies the wording on "gate": the jmap key `selectedBy` becomes
`gatedBy`.

Depends on: CR-011

Status: TESTS REVIEWED

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
- Double-buffered register:
- The gate applies the same way as to an undoubled register: a polled read is
`DataValidity::faulty` while the gate is closed.
- A closed gate must not wake a `wait_for_new_data` consumer of a
double-buffered register either. The same rule holds: neither wakes nor sees
a new version number.
- `wait_for_new_data` consumer:
- It must not observe a value while its gate is closed.
- Such a value is discarded, so the consumer neither wakes nor sees a new
version number.
- A suppressed delivery leaves the consumer's own state untouched: its value,
version number and data validity are those of its last delivered value.
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
register, numeric or ASCII
- the small `ClosedGateQuery` mixin through which the asynchronous
distributor reads the gate result
- the registration lookup recognises the ASCII accessor next to the numeric
base accessor
- `include/NumericAddressedBackendASCIIAccessor.h` (header-only): register
the ASCII reader with the low-level element's `ReadCoordinator`, so the gate
reaches it, and take its data validity from that registration
- `include/async/TriggeredPollDistributor.h` /
`src/async/TriggeredPollDistributor.cc`: `PolledAsyncVariable` reads the gate
result and suppresses the delivery through a closed gate.
- `include/NumericAddressedBackend.h` / `src/NumericAddressedBackend.cc`: wrap
the synchronous accessor of a gated register in `GatedByDecorator`.
- `include/NumericAddressedBackendMuxedRegisterAccessor.h` /
`src/NumericAddressedBackendMuxedRegisterAccessor.cc`: enforce the
register-level gate of a 2D register on the full-register accessor, which
the `GatedByDecorator` path does not reach.
- `include/DoubleBufferAccessor.h` / `src/DoubleBufferAccessor.cc`: carry the
gate query for the currently active inner buffer of a double-buffered gated
register.
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
- The element keys them by the gating register they read, not by the accessor
object.
- The element owns the gate accessors. The registration hands it the gating
register's path and the expected value; the element builds the accessor itself
with `getRegisterAccessor<uint64_t>(gatingRegisterPath, 0, 0, {})` for a gating
register it does not yet hold, and hands that accessor back. The registration
stores the accessor it is handed as the consumer's gate accessor. All consumers
of one gating register within one element therefore share the accessor the
element holds, which is the one that is read.
- Every accessor in the group is built by the element that owns it, so
`TransferGroup`'s once-and-never-cleared membership mark
(src/TransferGroup.cc:191-203) never meets a foreign accessor. A
re-registration on a transfer-group merge adds an accessor only for a gating
register the surviving element does not hold, so merged gated consumers are
served whatever their gating registers are.
- The gated accessor itself builds no gate accessor. The scalar precondition of
the gating register stays checked when the gated accessor is created (see
"Accessor construction").
- Gate accessors merge through the existing `replaceTransferElement()`
mechanism when their gating register is the same or adjacent. One gating
register is therefore read once per transfer, however many consumers it
gates.
- The group shares the ownership of the accessors with the registrations:
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
`getRegisterAccessor<uint64_t>(gatingRegisterPath, 0, 0, {})`, by the low-level
element that owns it (see "Gate in the low-level element").
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
- The registration of every gated consumer carries the gate: the gating
register's path and the expected value. It covers a gated scalar/1D accessor
and every gated channel slice of a 2D register. The element resolves the
accessor for that path.
- The full-register accessor of a 2D register with a register-level `gatedBy`
is not a registration consumer. It enforces the gate itself (see "2D
register-level gate").
- CR-011 registers every numeric read consumer of the low-level element,
strided or not. The gate is therefore attached through one registration path
instead of being special-cased per numeric conversion path. The ASCII reader
is not covered by that path and is gated explicitly (see "Gated ASCII
register").
- The accessor's `doPostRead()` copies two things into its own buffer:
- the payload, from the staging buffer
- the validity, from its registration
- `DoubleBufferAccessor` propagates the inner buffer's validity
(`src/DoubleBufferAccessor.cc:91-94`) and carries the gate query for a
double-buffered gated register (see "Gated double-buffered register").
- The accessor exposes its `_readRegistration` through a getter. The gate
result of its consumer can therefore be read from outside.
- `GatedBy` is the metadata type shared by the catalogue and the registration.

### 2D register-level gate

- A register-level `gatedBy` on a scalar or 1D register stays the register's own
gate, as today: the parser records it in the register's single channel and the
accessor's gate is attached to its registration. `registerGatedBy` is used
only by a 2D register.
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
- A double-buffered 2D register (e.g. `DAQ.FD`) never reaches this accessor:
its full-register accessor is a `DoubleBufferAccessor`, whose register-level
gate is enforced by the double-buffer mechanism (see "Gated double-buffered
register").
- `schemas/jmap.schema.json` no longer forbids `gatedBy` on a register with
`channels`.
- `doc/jmapFormat.dox` describes the register-level gate.

### Gated double-buffered register

- A register with `registerInfo.doubleBuffer` takes the double-buffer branch of
`getSyncRegisterAccessor()` (`src/NumericAddressedBackend.cc:234-246`), which
runs before the muxed branch and is not the plain-accessor gate wrap
(`src/NumericAddressedBackend.cc:209-211`).
- That branch attaches the gate query to the accessor it returns, so the gate
is enforced for a double-buffered register as well.
- `DoubleBufferAccessor::getInternalElements()` is empty
(`include/DoubleBufferAccessor.h:45`), so `GatedByDecorator` cannot resolve
the registration over that chain, and its constructor asserts a non-null
registration (`include/GatedByDecorator.h:78`).
- The gate query is therefore carried by the `DoubleBufferAccessor` itself, or
by an equivalent backend-side object:
- the `DoubleBufferAccessor` implements `ClosedGateQuery`
- the query answers for the currently active inner buffer
- The distributor's `_gateQuery` is non-null, so a closed gate neither wakes
the consumer nor advances the version number (see "Suppressing the
delivery").
- This also covers the full-register accessor of a double-buffered 2D
register. The guard `if(registerInfo.doubleBuffer == std::nullopt)`
(`src/NumericAddressedBackend.cc:162`) sends such a register to the
double-buffer branch, so its full-register accessor is a `DoubleBufferAccessor`
and never a `NumericAddressedBackendMuxedRegisterAccessor`. Its register-level
gate is enforced here, not in "2D register-level gate".

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

### Bit-range gated accessor

- A gated accessor can itself be a bit range (`registerInfo.isBitRange`):
- a scalar/1D register whose own representation has a non-zero `bitShift` and
carries a `gatedBy`
- a bit-field child slice of a gated channel (`doc/jmapFormat.dox:356-358`),
which carries `isBitRange` and the channel's `gatedBy`
- The plain (non-bit-range) scalar/1D accessor and the plain channel slice are
not bit ranges and are handled by the registration path above.
- A bit-range accessor needs its own handling:
- the `isBitRange` branch of `getSyncRegisterAccessor()`
(`src/NumericAddressedBackend.cc:165-191`) returns the
`BitRangeAccessorDecorator` before the plain-accessor gate wrap (`:209`), so
it is wrapped in `GatedByDecorator<UserType>` on that branch too, which keeps
the decorator outermost
- the bit-range target is built from a numeric-address path (`:184`), whose
register info carries no `gatedBy`
(`src/NumericAddressedRegisterCatalogue.cc:270-271`), so the gate is attached
to the target's base accessor explicitly instead of being read from its
register info
- the base accessor of a bit-range target is a
`NumericAddressedBackendRegisterAccessor<uint64_t, false>`
(`include/BitRangeAccessorDecorator.h:46`), whatever the accessor's `UserType`,
so the decorator cannot resolve the registration by `UserType`
- The gate is attached by re-registering the base accessor's consumer with the
gate: the gating register's path and the expected value. The base accessor
exposes an internal hook for this, so the gate reaches the coordinator without
a new core API.
- The decorator reads the gate result through a pointer to the consumer's gate
result flag. That flag lives in the heap-allocated consumer state, so the
pointer stays valid across a transfer-group merge. The plain path resolves it
from the base accessor's registration, the bit-range path from the `uint64_t`
base accessor it attached the gate to.
- A bit-range gating register is unaffected: the gating register never carries
a gate itself, so its accessor is not wrapped.

### Gated ASCII register

- The parser and the schema already accept a `gatedBy` on a register of data
type ASCII (`"string"`), so such a map declares a real gate. It is enforced.
- The accessor of such a register is a `NumericAddressedBackendASCIIAccessor`,
built by the ASCII branch of `getSyncRegisterAccessor()`
(`src/NumericAddressedBackend.cc:238-246`).
- That accessor currently bypasses the registration path: it owns a private
`_rawAccessor` (`include/NumericAddressedBackendASCIIAccessor.h:76`) and never
registers with the low-level element's `ReadCoordinator`, so the shared
enforcement point never sees its consumer and the gate is dropped.
- It therefore registers a read consumer with that coordinator, the same way
the numeric base accessor does, and the ASCII branch wraps it in
`GatedByDecorator<std::string>` when the register carries a `gatedBy`, so the
decorator is outermost as in the numeric branch
(`src/NumericAddressedBackend.cc:231-236`).
- The registration carries the gate: the gating register's path and the
expected value. The ASCII accessor takes its `DataValidity` from the
registration instead of from `_rawAccessor`, as the numeric accessor does.
- An equivalent explicit mechanism is acceptable, as long as the gate result
reaches the decorator and the ASCII accessor faults through it.
- The resulting behaviour:
- a polled read reports `DataValidity::faulty` while the gate is closed, with
an unspecified payload, and the data while the gate is open
- a `wait_for_new_data` consumer of the register is neither woken nor sees a
new version number while the gate is closed (the generic rules above)
- The accessor exposes its registration through the getter the numeric base
accessor uses. `GatedByDecorator` resolves the gate result from it; its lookup
recognises the ASCII accessor next to the two numeric `isRaw` instantiations.
- The creation-time checks are the same as for a numeric accessor: the gating
register must exist and be scalar (see "Accessor construction").
- The write path is unchanged. The gate applies to reads.

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
- `GatedByDecorator` reads the result through a pointer to the consumer's gate
result flag, resolved once at construction (the bit-range case is described in
"Bit-range gated accessor"). A transfer-group merge re-seats the registration
in place when it replaces the low-level element, so the pointer stays valid
while the accessor lives.
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
- It needs no member removal, because the element builds and keys its own gate
accessors by the gating register.
- No gate-specific member or behaviour is added to the class.
- The gate result, the `ClosedGateQuery` mixin and the registration getter are
backend-internal:
- the mixin is brought in only by `GatedByDecorator` and the backend's
`DoubleBufferAccessor`
- it is read only by `PolledAsyncVariable`
- the getter is on `NumericAddressedBackendRegisterAccessor`
- The gate query of a double-buffered gated register stays backend-internal
too: only `PolledAsyncVariable` reads it, so it is not a core
`NDRegisterAccessor`/`TransferElement`/`TransferGroup` change.

### Layer structure

The diagrams show the objects a register with `gatedBy` hands out. They show
who owns the gate result, and how the result travels to the object that acts
on it. The `read` edges are the read path going inward, the edges pointed back
out carry the returned data and the gate result, and the registration is the
member carrying the gate result.

The synchronous stack, outermost object first:

```mermaid
flowchart TB
  consumer["Consumer of the synchronous accessor<br/>- calls read()"]
  decorator["GatedByDecorator&lt;UserType&gt; (outermost)<br/>- the object the caller receives<br/>- transparent: forwards value, validity and version<br/>- brings in ClosedGateQuery::isGateClosed()<br/>- reads the gate result via the registration, resolved once at construction (dynamic_cast over getInternalElements())"]
  optional["BitRangeAccessorDecorator / SubArrayAccessorDecorator<br/>- optional, transparent"]
  base["NumericAddressedBackendRegisterAccessor&lt;UserType&gt; (base accessor)<br/>- one per consumer<br/>- owns the Registration member"]
  lowlevel["NumericAddressedLowLevelTransferElement<br/>- shared by every read consumer<br/>- owns the gate TransferGroup, read in the transfer phase"]
  coordinator["ReadCoordinator<br/>- per-consumer Consumer&lt;UserType&gt;<br/>- computes the GATE RESULT"]
  registration["Registration&lt;UserType&gt;<br/>- base accessor member<br/>- one per consumer<br/>- holds the GATE RESULT (open / closed)<br/>- written by the coordinator<br/>- not inferred from the value validity"]

  consumer -->|read| decorator
  decorator -->|read (delegated)| optional
  optional -->|read (delegated)| base
  base -->|read| lowlevel
  lowlevel -->|read| coordinator
  base -->|owns| registration
  coordinator -->|writes the GATE RESULT after each read| registration
  decorator -.->|value, validity, version| consumer
  registration -.->|gate result, read through the registration| decorator
```

The asynchronous case. The synchronous accessor in the middle is the same
stack as above, reached through the object
`NumericAddressedBackend::getSyncRegisterAccessor()` returns:

```mermaid
flowchart TB
  appconsumer["Consumer (application)<br/>- subscribes<br/>- receives pushed values"]
  polled["PolledAsyncVariable<br/>- inside TriggeredPollDistributor<br/>- one poll per distribute() call<br/>- resolves ClosedGateQuery once<br/>- overrides send(): gate closed and not initial: nothing is pushed; otherwise AsyncVariableImpl::send()<br/>- initial value by version: _lastSentVersion &lt; getVersionOnOpen()"]

  subgraph syncstack["the synchronous stack getSyncRegisterAccessor() returns, as above"]
    decorator["GatedByDecorator&lt;UserType&gt; (outermost)<br/>- the accessor the distributor polls<br/>- brings in ClosedGateQuery"]
    optional["BitRangeAccessorDecorator / SubArrayAccessorDecorator<br/>- optional, transparent"]
    base["NumericAddressedBackendRegisterAccessor&lt;UserType&gt; (base accessor)<br/>- one per consumer<br/>- owns the Registration member"]
    lowlevel["NumericAddressedLowLevelTransferElement<br/>- shared by every read consumer<br/>- owns the gate TransferGroup, read in the transfer phase"]
    coordinator["ReadCoordinator<br/>- per-consumer Consumer&lt;UserType&gt;<br/>- computes the GATE RESULT"]
    registration["Registration&lt;UserType&gt;<br/>- holds the GATE RESULT"]
    decorator -->|read (delegated)| optional
    optional -->|read (delegated)| base
    base -->|read| lowlevel
    lowlevel -->|read| coordinator
    base -->|owns| registration
    coordinator -->|writes the GATE RESULT| registration
  end

  appconsumer -->|subscribe| polled
  polled -->|poll / read| decorator
  registration -.->|gate result through the registration| decorator
  decorator -.->|isGateClosed(): gate result| polled
  polled -.->|send| appconsumer
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
- a gating register that is itself a bit range (its representation has a
non-zero `bitShift`) is accepted and gates correctly; the fixture declares a
genuine bit range, so the test exercises a bit-range gating register
- an unsigned gate value with the top bit set gates correctly
- the parser rejects a negative `value`
- Gated bit-range accessors:
- a scalar/1D register whose own representation has a non-zero `bitShift` and a
`gatedBy` reports `DataValidity::faulty` while its gate is closed and the data
while it is open
- a bit-field child slice of a gated channel reports `DataValidity::faulty`
while the channel's gate is closed
- the async path of a gated bit-range accessor neither wakes nor advances the
version number while its gate is closed
- Gated ASCII (`string`) register (fixture with a `"string"` register and a
`gatedBy`):
- a polled read reports `DataValidity::faulty` while its gate is closed and
the data while it is open
- the async path, if supported for ASCII, neither wakes the consumer nor
advances the version number while the gate is closed
- Polled 2D (`testMultiplexedDataAccesor.cpp`):
- per-channel gating of the slices within one read
- register-level whole-block gate on the full-register accessor, enforced
in `NumericAddressedBackendMuxedRegisterAccessor`
- independence of the two gates on a register that uses both
- Async (`testAsyncRead.cpp`, new fixture `tests/gatedByInterrupt.jmap`):
- an open-gate interrupt delivers a new value and advances the version number
- a closed-gate interrupt neither wakes the consumer nor changes its value,
version number or data validity, so the expectations after a suppression are
those of the last delivered value, not of the initial one
- a change of the gating register while the consumer is pending
- the consumer never observes the data of a closed alternative
- Gated double-buffered registers:
- a gated double-buffered scalar/1D reports `DataValidity::faulty` while its
gate is closed and the data while it is open
- a gated double-buffered channel slice (e.g. `DAQ.FD.AmplitudeCh0`) reports
`DataValidity::faulty` while its gate is closed and the data while it is open
- a closed gate neither wakes the async consumer of a double-buffered register
nor advances its version number
- a register-level gate on a double-buffered 2D register faults the
full-register accessor while the gate is closed
- Merged gated consumers whose gating registers differ: two gated registers at
one address gated by different gating registers are obtained as
`wait_for_new_data` consumers together and are served by the merged element
without a TransferGroup error, each honouring its own gate
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

- Wrapping the `DoubleBufferAccessor` in `GatedByDecorator`: rejected.
- `DoubleBufferAccessor::getInternalElements()` is empty
(`include/DoubleBufferAccessor.h:45`), so `findBaseRegisterRegistration()`
(`include/GatedByDecorator.h:60-66`) finds nothing and the decorator's
constructor asserts a non-null registration (`include/GatedByDecorator.h:78`):
an assert in debug, a null dereference in release.
- The double-buffer branch carries the gate query on the accessor itself
instead (see "Gated double-buffered register").

- Gate enforcement in a decorator on the synchronous accessor, with a shared
`GatedByState` handshake: rejected.
- The gate must be enforced where the raw data is converted.
- That element is shared by consumers with different gates.
- Preparing payload and validity there is one implementation instead of two.
- It removes the shared-state handshake as well.
- The new `GatedByDecorator` only reads the result; it enforces nothing.
- An inner `TransferGroup` in the low-level element deduplicating on the
accessor object: rejected.
- `TransferGroup` marks a member once and never clears it
(src/TransferGroup.cc:191-203) and has no removal API, so after a
transfer-group merge the surviving element's group would be handed the
abandoned element's gate accessor for the same gating register and reject it.
- The element keys its gate set by the gating register and hands its held
accessor back to the registration instead.
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
only ever recorded, never enforced. It did ship in the release 03.31.00, where
`doc/jmapFormat.dox` documents the key and `schemas/jmap.schema.json` defines
it as part of format `"1.0"`. A map carrying the old key still loads, because
the parser reads only the keys it knows and ignores the rest, so its gate
metadata is silently dropped and the register stays ungated; the schema rejects
such a file. The format version therefore stays `"1.0"` and no compatibility
shim is added.
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

- DI-1 [DONE] Register-level `gatedBy` on a 2D register: the plan changes the documented behaviour. Today the parser applies a register-level `selectedBy` on a 2D register as the default for all channels (doc/jmapFormat.dox:406, tests/selectedByInheritance.jmap:112, tests/executables_src/testJsonMapFileParser.cpp:1283/1291), but CR-016 says it is 'recorded separately ... independent of the per-channel gates' and gates the whole block on the full-register accessor, while the 'Renamed forma...
- DI-2 [DONE] The full-register accessor of a 2D register is a `NumericAddressedBackendMuxedRegisterAccessor` (include/NumericAddressedBackendMuxedRegisterAccessor.h:106 getInternalElements() == {}, :108 replaceTransferElement() no-op, own doReadTransferSynchronously; CR-011 serves only the channel slices). It has no low-level element and no `ReadCoordinator` registration, so the planned `GatedByDecorator`+registration mechanism (and the 'the full-register accessor of a 2D register with a...
- DI-3 [DONE] The Requirements claim `NumericAddressedRegisterInfo::registerSelectedBy` becomes `registerGatedBy`, but no such member exists anywhere in the code (src/JsonMapFileParser.cc stores the register-level gate only in `Channel::selectedBy`/`info.channels.back().selectedBy`). The Specifications correctly call `registerGatedBy` a new member, so the Requirements wording is factually wrong and internally inconsistent.
- DI-4 [DONE] The plan does not extend the existing parser validations to the new register-level `registerGatedBy`: the checks that a selector register exists, that it is not itself gated, and that `selectedBy` is used only on read-only registers iterate `reg.channels[].selectedBy` (src/JsonMapFileParser.cc:695-721 and 365-378). Stored separately, a register-level gate would bypass them, accepting an undefined or gated selector register.
- DI-5 [DONE] Specifications lists a non-existent affected component `src/NumericAddressedLowLevelTransferElement.cc`; `NumericAddressedLowLevelTransferElement` is header-only (include/NumericAddressedLowLevelTransferElement.h, 212 lines) and no such .cc exists anywhere in the tree (also absent in CR-011, which named only the header).
- DI-6 [DONE] Requirements claim the word 'selection' is eliminated, but Specifications state the `chimeratk-map-to-jmap` tool is unaffected, while tools/chimeratk-map-to-jmap:618 (`_build_2d_descriptor` docstring) still names the key `selectedBy`; the two statements conflict and a stale mention survives the rename scope.
- DI-7 [DONE] Rename scope leaves 'selection' survivors, contradicting Requirements ('The word "selection" is gone', CR-016:92) and the plan's own 'no selectedBy survives anywhere' (CR-016:97). The Specifications doc bullet (CR-016:142-146) names only the anchors jmap_channel_selection and jmap_module_selection_inheritance, but doc/jmapFormat.dox also has the heading/anchor 'Conditional register selection' / jmap_register_selection (doc/jmapFormat.dox:137) plus its cross-reference \ref jm...
- DI-8 [DONE] Same 'selection' survivor class in the schema: schemas/jmap.schema.json:326 ('Selects this channel by writing a value to a selection/mux register'), :397 and :401 ('selection/mux register') keep the word 'selection'/'Selects'. The schema bullet (CR-016:134-138) only says 'rename the key and the type' and 'reword the diagnostics'; the description strings are not covered.
- DI-9 [DONE] Gate accessor type is stated inconsistently: 'Gate in the low-level element' (CR-016:189) types the gate as 'std::shared_ptr<ScalarRegisterAccessor<uint64_t>>', but the plan obtains it with getRegisterAccessor<uint64_t> (CR-016:231), which returns boost::shared_ptr<NDRegisterAccessor<uint64_t>> in this codebase (src/NumericAddressedBackend.cc:156-158). ScalarRegisterAccessor is the user-facing abstractor returned by Device::getScalarRegisterAccessor, not by getRegisterAccess...
- DI-10 [DONE] Minor editorial inconsistency: 'Renamed format' (CR-016:162-165) says the three fixtures selectedByInheritance.jmap, simpleJsonFile.jmap and muxedPolled.jmap are 'renamed and rewritten', but only selectedByInheritance.jmap is actually renamed (to gatedByInheritance.jmap, Test plan CR-016:506); the other two keep their names and are only rewritten.
- DI-11 [DONE] Async gate enforcement is bypassed: PolledAsyncVariable<UserType>::send() (include/async/TriggeredPollDistributor.h:127-136) computes isInitialValue from its own _lastSentVersion, but sets it with '_lastSentVersion = this->_sendBuffer.versionNumber;' AFTER the base AsyncVariableImpl::send() (include/async/AsyncAccessorManager.h:228-240) has already destructively moved the buffer via subscriber->sendDestructively(_sendBuffer). The buffer's versionNumber is reset to v0 by tha...
- DI-12 [DONE] Two gated registers that share a hardware address cannot be obtained as wait_for_new_data consumers: obtaining the second gated accessor throws 'ChimeraTK::logic_error: The given accessor is already in a TransferGroup and cannot be added to another.' (thrown in TransferGroup::addAccessorImpl, src/TransferGroup.cc:189-193). Cause: each gated payload accessor builds its own gate accessor (src/NumericAddressedBackendRegisterAccessor.cc:92) and hands it to its raw element's gat...
- DI-13 [DONE] DI-12 is a specification defect, not just an implementation bug. CR-016:201-220 (spec) mandates that the low-level element collect its consumers' gate accessors in a TransferGroup of its own, and claims at CR-016:209-210 that 'the re-registration of a transfer-group merge adds nothing' because the group 'is add-only' and 'skips one the group already holds'. That claim only holds for identity dedup within one element: a transfer-group merge moves a *different* gate accessor ...
- DI-14 [DONE] testAsyncReadGatedBy (new test, tests/executables_src/testAsyncRead.cpp) has residual failures that are test-expectation bugs, not implementation defects: line 826 compares gated.getVersionNumber() against versionAfterInitial captured after the *first* delivery, but a second open-gate interrupt (0x5678) legitimately advanced the version (line 807 only asserts '>' versionAfterInitial); line 827 expects dataValidity()==faulty although the accessor was last updated by a valid de...
- DI-15 [DONE] Specifications, 'Gate in the low-level element' (CR-016:201-215): the plan makes the low-level element collect its consumers' gate accessors in a `TransferGroup` of its own, deduplicated on the accessor object, and claims the re-registration of a transfer-group merge 'adds nothing'. That claim is wrong. After the merge of two gated payload elements at one address (include/async/TriggeredPollDistributor.h:104 via replaceTransferElement, src/NumericAddressedBackendRegisterAcc...
- DI-16 [DONE] Internal inconsistency: 'Alternatives considered' (CR-016:573-578) rejects 'an inner TransferGroup in the low-level element owning the gate accessors' and names 'the registration owns the gate accessor instead' as the choice, while the Specifications (CR-016:201-215) mandate exactly that inner TransferGroup. The rejected alternative must be restated as the accessor-identity-based dedup, which is what actually fails.
- DI-17 [DONE] Specifications, '2D register-level gate' (CR-016:269-295): the plan never states that a register-level `gatedBy` on a scalar or 1D register stays the register's own gate. src/JsonMapFileParser.cc:409-417 stores it in the register's single channel and tests/simpleJsonFile.jmap:589-593 documents that case, so a re-implementation could route a scalar register's gate into `registerGatedBy` and lose the gate on the scalar accessor.
- DI-18 [DONE] DI-14: the plan nowhere states that a suppressed (closed-gate) delivery leaves the consumer's value, version number and data validity as of its last delivered value; the async test bullets (CR-016:548-550) leave the opposite expectation open, and tests/executables_src/testAsyncRead.cpp:825-827 expresses it (version compared against the initial delivery, validity expected faulty although the last delivery was valid).
- DI-19 [DONE] Specifications 'Gate in the low-level element' still breaks on merged gated consumers with different gating registers. The folded rule (element keys its gate accessors by the gating register and hands the held accessor back) only avoids the TransferGroup conflict when the surviving element already holds the same gating register. Two gated payload accessors at one address with different gating registers (legal input: two read-only registers at one offset, as in tests/gatedBy...
- DI-20 [DONE] The format-version decision in 'Alternatives considered' rests on a justification the repository contradicts in part: 'the feature was only ever recorded, never enforced, so no map content carrying the old key is in use'. `selectedBy` did ship in the release tag 03.31.00 (2026-10-13), where doc/jmapFormat.dox documents it and schemas/jmap.schema.json defines it as part of format 1.0. After the rename such a map still loads, because src/JsonMapFileParser.cc reads only the ke...
- DI-21 [DONE] include/async/AsyncAccessorManager.h:178: `AsyncVariableImpl<UserType>::send()` is declared `void send();`, while the base commit had `void send() final;`. CR-016 removed `final` (so that `PolledAsyncVariable::send()`, include/async/TriggeredPollDistributor.h:127, can override it) but did not add `override`, so `AsyncVariableImpl::send()` now overrides the pure virtual `AsyncVariable::send()` (AsyncAccessorManager.h:23) unmarked. clang-tidy reports `clang-diagnostic-inconsi...
- DI-22 [DONE] Lowercase unsigned-integer literal suffixes on new CR lines: tests/executables_src/testAsyncRead.cpp:799,807,830,840,879,893,935,948,993,1005 and tests/executables_src/testNumericAddressedBackendRegisterAccessor.cpp:853,859. clang-tidy reports `readability-uppercase-literal-suffix` for each; the rest of both files uses the uppercase `U` suffix, so the new literals (e.g. `0x1234u`, `0x80000000u`, `0x7fffffffu`) deviate from the file convention. Fix: uppercase the suffix.
- DI-23 [DONE] tests/executables_src/testNumericAddressedBackendRegisterAccessor.cpp:889: the new private member `readCount` lacks the leading underscore required by `readability-identifier-naming.PrivateMemberPrefix` (compare `_registerer` in the same class); clang-tidy reports `readability-identifier-naming` on this new line. Fix: rename to `_readCount`.
- DI-24 [DONE] include/ReadCoordinator.h:257 and :259: the new `Consumer` constructor parameters produce clang-tidy findings: `performance-unnecessary-value-param` on `RegisterPath consumerGatePath` (:257) and `performance-move-const-arg` on `gatePath(std::move(consumerGatePath))` (:259, 'passing result of std::move() as a const reference argument; no move will actually happen'). Take `RegisterPath` by value only if moved into a value member, otherwise by const reference, and drop the ine...
- DI-25 [DONE] src/JsonMapFileParser.cc:318-330: the 2D-register branch carries a duplicated comment block - two consecutive comments state the same rule ('a register-level gate is recorded separately and gates only the full-register accessor; it is not inherited by the channels') with slightly different wording. Collapse them into one comment.
- DI-26 [DONE] include/NumericAddressedRegisterCatalogue.h:42 (new doxygen comment) - typo 'faultied' in '...the gate is closed and its data is faultied.'; should be 'faulty'. It is the only occurrence of 'faultied' in the tree.
- DI-27 [DONE] include/NumericAddressedRegisterCatalogue.h:48 (new doxygen comment on GatedBy::regPath) - broken grammar 'that determines if a Channel/Register is gate open'; should read e.g. '...that determines whether the gate of a Channel/Register is open'.
- DI-28 [DONE] include/NumericAddressedLowLevelTransferElement.h:11 - the CR adds '#include <set>', but <set> is unused (the header uses only std::map, _gateAccessors; no std::set anywhere in the file). Remove it (include-what-you-use; clang-tidy's misc-include-cleaner does not report unused includes in headers, so this was found by inspection).
- DI-29 [DONE] tests/executables_src/testJsonMapFileParser.cpp:1046 (new comment) - American spelling 'initialized' ('Two identically initialized infos are equal...') against the project's British-English rule; the codebase predominantly uses 'initialise'/'initialised' (incl. the isInitialised() identifiers). Use 'initialised'.
- DI-30 [DONE] tests/executables_src/testMultiplexedDataAccesor.cpp:567 - the comment added by this CR ('// Reopen the register-level gate but change the channel selection: the two gates move independently.') uses the word 'selection'. CR-016 requires the selection wording to disappear (Requirements: 'The word "selection" is gone', 'no `selection` wording survives', CR-016:92-94 and :142-143). Reword to the gate terminology, e.g. '...but change the channel gate: ...'.
- DI-31 [DONE] tests/simpleJsonFile.jmap:246 - the fixture description rewritten by this CR still reads 'For registers whose layout is selected by a register value (multiple DAQ tabs), a \'gatedBy\' dict is added...'. This CR changes that very line (it renames the key inside it) and requires the selection wording to be gone, so the sentence must use the gate wording, e.g. 'For registers whose layout is gated by a register value ...'. Same survivor class as resolved deferred issue DI-7/DI-...
- DI-32 [DONE] A gated accessor whose registerInfo.isBitRange is true is never gated at runtime. src/NumericAddressedBackend.cc:165 takes the isBitRange branch and returns the BitRangeAccessorDecorator (src/NumericAddressedBackend.cc:187/190) before the GatedByDecorator wrap at :209; its target is built from a numeric-address path at :184, whose register info (src/NumericAddressedRegisterCatalogue.cc:270) carries no gatedBy, so the gate is dropped. This affects (a) a gated 2D channel that...
- DI-33 [DONE] tests/gatedByScalar.jmap:14-18 declares CTRL.BR_SEL 'as a bit range (bits 0..3)' with bitShift: 0, which does not make it a bit range: src/JsonMapFileParser.cc:183 sets isBitRange only for bitShift != 0, and :723-731 reclassifies only a register that shares its address with a bit range. No register in the fixture is a bit range, so TestGatedByBitRangeGatingRegisterPolled (tests/executables_src/testNumericAddressedBackendRegisterAccessor.cpp:822) does not exercise a bit-rang...
- DI-34 [DONE] DI-32 unresolved in code: the isBitRange branch of NumericAddressedBackend::getSyncRegisterAccessor() (src/NumericAddressedBackend.cc:164-192) still returns the BitRangeAccessorDecorator (src/NumericAddressedBackend.cc:187/190) without wrapping it in GatedByDecorator<UserType>, so a gated accessor whose registerInfo.isBitRange is true (a gated scalar/1D register with a non-zero bitShift, or a bit-field child slice of a gated channel) is never gated at runtime. The plain-acc...
- DI-35 [DONE] DI-33 unresolved in code: tests/gatedByScalar.jmap:14-18 declares CTRL.BR_SEL 'as a bit range (bits 0..3)' with bitShift: 0, which does not make it a bit range (src/JsonMapFileParser.cc:183 sets isBitRange only for bitShift != 0; :723-731 reclassifies only a register sharing its address with a bit range). No register in the fixture is a bit range, so TestGatedByBitRangeGatingRegisterPolled (tests/executables_src/testNumericAddressedBackendRegisterAccessor.cpp:822) does not ...
- DI-36 [DONE] Gated registers that are also double-buffered are not covered by the plan, and no async delivery is suppressed for them. NumericAddressedBackend::getSyncRegisterAccessor() takes the double-buffer branch (src/NumericAddressedBackend.cc:234-246) for any register with registerInfo.doubleBuffer set, before the muxed branch and without the GatedByDecorator wrap of the plain branch (:209-211); DoubleBufferAccessor is not a ClosedGateQuery, so PolledAsyncVariable::_gateQuery is nu...
- DI-37 [DONE] A 2D double-buffered register never reaches the plan's 2D register-level gate enforcement point. The guard 'if(registerInfo.doubleBuffer == std::nullopt)' at src/NumericAddressedBackend.cc:162 sends such a register to the double-buffer branch, so its full-register accessor is a DoubleBufferAccessor and never a NumericAddressedBackendMuxedRegisterAccessor; the plan's '2D register-level gate' section (CR-016:286-315) and its Specifications entry (CR-016:127-130) place that en...
- DI-38 [DONE] src/NumericAddressedBackend.cc:269 - the new comment 'A double-buffered register cannot take the plain-accessor gate wrap (`:231`) or the muxed branch, so its gate ...' embeds a bare line-number cross-reference `(`:231`)`. No pre-CR source comment in the tree uses line-number references (checked at the base commit), the number is brittle (it drifts as the file changes), and it does not even point at the cited construct: the plain-accessor gate wrap sits at src/NumericAddres...
- DI-39 [DONE] A gated ASCII (`"string"`) register is silently left ungated, so the requirement "a polled read of a register with a `gatedBy` is `DataValidity::faulty` while the gate is closed" (Requirements, CR-016:25-31) and the specification's claim that the gate "is attached uniformly instead of being special-cased per conversion path" (CR-016:283-285) hold for numeric registers only. `NumericAddressedBackend::getSyncRegisterAccessor()` has a dedicated ASCII branch (src/NumericAddress...
- DI-40 [DONE] No other deviations found: the implemented gate mechanism matches the plan for numeric scalar/1D, bit-range, 2D register-level and per-channel, double-buffered and async paths (verified in src/NumericAddressedBackend.cc:156-285, include/NumericAddressedLowLevelTransferElement.h, include/ReadCoordinator.h:238-319, src/NumericAddressedBackendRegisterAccessor.cc:80-114/344-402, src/NumericAddressedBackendMuxedRegisterAccessor.cc:137-189, src/DoubleBufferAccessor.cc:21-126, include/as...
- DI-41 [DONE] tests/executables_src/testAsyncRead.cpp:869, :882, :892 - the three new shift expressions `0x5U << 4`, `0x9U << 4` and `0xaU << 4` in testAsyncReadGatedByBitRangeAccessor carry a signed shift count (the literal `4`), so clang-tidy reports `hicpp-signed-bitwise` ('use of a signed integer operand with a binary bitwise operator') on each of these new lines. The project config enables the check and the no-warnings policy forbids new linter warnings; tests in this tree predomina...
- DI-42 [NEW] The 'Layer structure' subsection of CR-016 (the synchronous-stack diagram and the asynchronous-case diagram) uses ASCII-art boxes; the user requires mermaid graphs instead of ASCII art.
