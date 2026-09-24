# Runtime `selectedBy` Enforcement in the `NumericAddressedBackend`

Synopsis: The map-file parsing of `selectedBy` is already complete — every register
and channel carries an effective `NumericAddressedRegisterInfo::ChannelInfo::selectedBy`
(a `regPath` + `val`), propagated recursively from modules/register-modules and defaulted
onto 2D channels — but the value is currently dead metadata. This change adds the
**runtime enforcement** so that a register declared `selectedBy` is only reported as
*active* while the selected register equals the configured value, for both polled
accessors and interrupt-driven (`wait_for_new_data`) accessors. Reads are gated per
register/channel-set, so mutually exclusive mux alternatives (e.g. DAQ tabs) activate
independently. `selectedBy` may only be used on read-only registers, which removes the
entire write path from consideration.

Status: IN REVIEW

## Requirements

Aspect: runtime enforcement of `selectedBy` on registers of the `NumericAddressedBackend`.

- **Read-only restriction**: `selectedBy` may only be applied to registers with
  `Access::READ_ONLY` or `Access::INTERRUPT`. Applying it to a writable register
  (`Access::READ_WRITE` or `Access::WRITE_ONLY`) is a **parsing error** and the map
  file parser rejects the input. Rationale: `selectedBy` describes *when* data is valid
  to read; writable registers imply mutable state, which conflicts with the semantics of
  conditional availability.
- **Inherited `selectedBy` validation**: a module or parent register declaring
  `selectedBy` is only legal if **all descendant registers** inheriting it are read-only
  (`Access::READ_ONLY` or `Access::INTERRUPT`); otherwise the parser rejects the map with
  an error pointing to the parent declaration, so the constraint is checked at the source
  of the inheritance, not at individual children.
- **Polled semantics** *(decided: `DataValidity::faulty`)*: a plain `read()` of a gated
  register only performs the physical read (the address may be shared with an
  alternative; reading is harmless) when the selctor matches. The returned data is marked
  `DataValidity::faulty` when the selector register does not match. The payload is
  **unspecified** when unselected (see *Payload when unselected* below).
- **2D per-channel gate**: each channel of a muxed 2D register is gated by its own
  effective `selectedBy` (its own, falling back to the register-level default / inherited
  value). An inactive channel is marked `DataValidity::faulty` while the selected
  channels of the same read remain valid. Because `doPostRead()` does not overwrite
  inactive channels with the inactive layout, the buffer typically still holds prior
  data, but the inactive channel's payload is also **unspecified** (see *Payload when
  unselected* below). This matches the muxed-layout example where `AmplitudeCh0` (sel 0)
  and `RawCh0` (sel 1) coexist in one register: in a single read only the
  currently-selected channel set is valid.
- **Interrupt semantics (per subscription)**: `wait_for_new_data` consumers must **not
  wake** while their selection is not met. Gate evaluation is per `AsyncVariable` (per
  subscribed register/channel-set), not per interrupt domain, because one interrupt
  domain may contain registers or channel slices with different selections that must
  wake independently (`DAQ.FD/AmplitudeCh0`, sel 0, vs `DAQ.FD/RawCh0`, sel 1).
  An unselected subscription is delivered with an **unchanged version number** and
  `DataValidity::faulty`, so the waiting consumer stays pending. The delivered payload is
  **unspecified** when unselected (see *Payload when unselected* below).
- **Payload when unselected** *(uniform contract)*: whenever a read or subscription is
  gated inactive, `DataValidity::faulty` is the only guaranteed signal; the payload is
  **unspecified**. In practice the buffer most likely still holds the last known good
  data (e.g. inactive channels of a muxed 2D register or an unselected scalar are not
  deliberately overwritten), but the buffer may be destroyed and the payload may carry
  the inactive alternative's data, so consumers **must not** trust payload bytes while
  unselected — they must gate on `DataValidity::faulty` (and, for `wait_for_new_data`,
  on the version number). "Keep last good" is an incidental property, **not** a
  guarantee, and is therefore **not** part of the test contract.
- **Initial value**: on `activateAsyncRead`, the initial value is delivered immediately
  but marked `DataValidity::faulty` if the selection is not yet met *(decided)*; once the
  selector matches and new data arrives, a valid value follows.
- **Scope of `wait_for_new_data`**: only the *full* register should advertise it when a
  `selectedBy` is present; the fixed `BUF0`/`BUF1` views already must not (see CR-001).
  Concretely: a double-buffered data register carrying a `selectedBy` (e.g. `DAQ.DATA`,
  double-buffer + pull mode in the test plan below) exposes the interrupt-driven
  `ChimeraTK::AccessMode::wait_for_new_data` accessor only on the **full register** path
  — the subscription that a consumer makes is therefore always on the register itself,
  never on a buffer view. The fixed `BUF0`/`BUF1` views remain plain double-buffer slices:
  they are excluded from `wait_for_new_data` regardless of `selectedBy` (this exclusion
  predates this change and is tracked as CR-001), and they are not selection-gated
  themselves, because the gate is evaluated per subscription on the full-register accessor.
  This keeps the interrupt surface narrow — only the full register (and its generated
  channel slices such as `DAQ.FD/AmplitudeCh0`) becomes a selection-gated interrupt
  consumer — while the buffer views stay passive. No new `wait_for_new_data` capability
  is introduced for `BUF0`/`BUF1`.
- **Selector register access**: the selector's `regPath` is a fully qualified register
  path in the same catalogue. It is read through a synchronous scalar accessor
  (`getSyncRegisterAccessor<int64_t>`), must be byte-aligned and ≤ 64 bit wide, and must
  be cheap: one 32/64-bit read per gate check, without copying the whole data block when
  inactive. Selectors are concrete integer values (≤ 64 bits) and must form a disjoint set
  (mutually exclusive). Bitmask-based selection is out of scope.
- **No write-path changes**: because `selectedBy` is restricted to read-only registers,
  no gating is needed on any write path.

## Specifications

Affected components:

- New `include/SelectorGate.h` / `src/SelectorGate.cc`: reusable selector read/compare
  plus the `DataValidity` policy.
- `include/NumericAddressedBackendRegisterAccessor.h` / `src/...cc`: gate scalar/1D
  reads.
- `include/NumericAddressedBackendMuxedRegisterAccessor.h` / `src/...cc`: per-channel
  layout gate + per-channel `DataValidity` for 2D reads.
- `include/DoubleBufferAccessor.h` / `src/DoubleBufferAccessor.cc`: gate double-buffer
  reads on the active selection.
- `include/async/TriggeredPollDistributor.h` / `src/async/TriggeredPollDistributor.cc`:
  per-subscription wake/version gating for the interrupt path.
- `include/BackendRegisterCatalogue.h` (class `BackendRegisterCatalogueBase`),
  `include/NumericAddressedRegisterCatalogue.h`
  / `src/NumericAddressedRegisterCatalogue.cc`: per-register `selectedBy` lookup
  (`getSelectedBy`).
- `src/NumericAddressedBackend.cc` (function `getSyncRegisterAccessor`): thread the gate
  into the accessor constructors.
- `src/JsonMapFileParser.cc`: enforce the read-only constraint.
- `doc/jmapFormat.dox`: document runtime gating + the read-only restriction.
- `tests/...`: new tests and fixtures (see Test plan), plus an audit of existing jmap
  fixtures (see below).

Selector gate (`SelectorGate`):

- A small class holding a `ScalarRegisterAccessor<int64_t>` for the selector register,
  the expected value, and the validity policy:
  `bool check()` (read selector, compare), `void replace(backend, SelectedBy,
  forceFirstFaulty)`, `DataValidity dataValidity()` (ok / faulty per the policy).
- Reuses the existing `ScalarRegisterAccessor` /
  `getSyncRegisterAccessor<int64_t>` so no new I/O primitive is needed.

Scalar / 1D accessor (`NumericAddressedBackendRegisterAccessor`):

- Hold an optional `SelectorGate` built from `registerInfo.channels[0].selectedBy`.
- In `doReadTransferSynchronously()` still always perform the physical read, then in
  `doPostRead()` apply the gate to set `DataValidity` per the polled semantics above.
- No write-path changes are needed (`selectedBy` is restricted to read-only registers).

2D muxed accessor (`NumericAddressedBackendMuxedRegisterAccessor`):

- `doPostRead()` currently demultiplexes every channel unconditionally; add a
  **per-channel gate**: if a channel's selector matches, convert/demux its data normally
  and keep `DataValidity::ok`; if it does not match, mark that channel
  `DataValidity::faulty` and do not overwrite it with the inactive layout (the channel's
  payload is unspecified when inactive, per *Payload when unselected* above).
- Per-channel validity is propagated through `buffer_2D` and the accessor's per-channel
  validity, so consumers see exactly which channels are active.

Double-buffered accessor (`DoubleBufferAccessor`):

- Double-buffered data registers are the classic interrupt-triggered muxed case (one
  firmware data block, switchable content selected by a control register). Add an
  optional `SelectorGate`; the gate decides whether the buffer just read is the active
  one, and if not the read is treated as not-new (see the async integration below).

Async interrupt integration (per subscription — main focus):

- The gate is evaluated **per `AsyncVariable`**, which is the granularity at which the
  interrupt data is delivered: `createAsyncVariable()` (`TriggeredPollDistributor.h:71`)
  builds a per-variable `SelectorGate` from the catalogue lookup `getSelectedBy(...)`
  (§Catalogue below) and adds the selector register accessor to `_transferGroup`.
  TransferGroup deduplicates by TransferElement, so one selector register is read at most
  once per poll, shared by all variables choosing the same selector.
- Two levels:
  1. **Validity level** — the variable's synchronous accessor applies its selector gate
     in `doPostRead` and marks the inactive channel(s) `DataValidity::faulty`;
     `PolledAsyncVariable::fillSendBuffer()` (`TriggeredPollDistributor.h:94`) already
     forwards `_syncAccessor->dataValidity()`, so inactive data is delivered as faulty
     with no further async code. This level is shared by the polled and the interrupt
     read paths.
  2. **Wake/version level** — after `_transferGroup.read()`, each `PolledAsyncVariable`
     evaluates its own gate: unselected ⇒ deliver with the *previously published* version
     number and force `DataValidity::faulty` (unchanged version ⇒ `wait_for_new_data`
     consumers do not wake with the inactive alternative); selected ⇒ deliver with the
     domain version from the DataConsistencyRealm and the accessor's (already gated)
     validity.
- Implement the per-variable state in `PolledAsyncVariable::fillSendBuffer()`; **do not**
  signal "unselected" by returning `false` from `prepareIntermediateBuffers()`. Per the
  contract (`AsyncAccessorManager.h:149-156`) a `false` return means the read failed and
  *must* call `setException()`; "unselected" is not an error. `prepareIntermediateBuffers()`
  keeps returning `true`, and its domain-global `_forceFaulty`/version machinery stays
  reserved for genuine DataConsistencyRealm staleness only.
- Note: a **full-register** subscription to a muxed 2D register (`DAQ.FD`) has no
  register-level `selectedBy`, hence no wake suppression; per-channel validity governs
  (§2D accessor above). This is exactly the "different channel sets" use case.
- Selector-chain recursion (a selector register that is itself conditionally selected) is
  out of scope (see Alternatives).

Catalogue lookup (`getSelectedBy`):

- Add to the **base** catalogue
  (`include/BackendRegisterCatalogue.h`, class `BackendRegisterCatalogueBase`):
  `virtual std::optional<SelectedBy> getSelectedBy(const RegisterPath&) const`, defaulting
  to `std::nullopt`; overridden by `NumericAddressedRegisterCatalogue`. Keeping it on the
  base keeps `TriggeredPollDistributor` (shared with the LMap/PCIe backends) generic;
  other backends simply return `nullopt` (no gating).
- `NumericAddressedRegisterCatalogue::getSelectedBy(path)` returns the register's
  *effective* selection directly from the already-propagated `ChannelInfo.selectedBy`
  (`NumericAddressedRegisterCatalogue.h:63`):
  - scalar/1D register (including generated channel slices like `DAQ.FD/AmplitudeCh0`,
    whose `selectedBy` the parser copies at `JsonMapFileParser.cc:464-465`) →
    `channels[0].selectedBy`;
  - full 2D muxed register → `std::nullopt` (no single gate; the per-channel values are
    only relevant inside the accessor);
  - double-buffer `BUF0`/`BUF1` views → they are plain slices of the register they view
    and inherit the same `ChannelInfo` (`JsonMapFileParser.cc:399-411`), so no special
    case.
- This is a pure lookup of metadata the parser already computed — no domain→register
  back-mapping and no `getQualifiedAsyncId` coupling, hence no ambiguity when a domain
  mixes registers with different selections. `NumericAddressedBackend::activateSubscription`
  (`NumericAddressedBackend.cc:267`) is unchanged: the gate is built per subscription
  inside `createAsyncVariable`, which already has the descriptor.

Backend accessor construction (`getSyncRegisterAccessor`):

- A single place to route based on `selectedBy` (`src/NumericAddressedBackend.cc:155-242`):
  - scalar/1D with a `selectedBy` → `NumericAddressedBackendRegisterAccessor` with the
    gate supplied;
  - 2D / default-selected → `NumericAddressedBackendMuxedRegisterAccessor` (per-channel
    gates);
  - double-buffer → `DoubleBufferAccessor` with the gate.
- No structural change; just thread an optional `SelectedBy`/`SelectorGate` through the
  existing constructors.

Parser read-only enforcement (`JsonMapFileParser`):

- Throw `ChimeraTK::logic_error` if `selectedBy` is used on a writable register,
  including inherited cases (the error points to the parent declaration), per the
  requirements above.

Documentation (`doc/jmapFormat.dox`):

- Document the runtime behaviour / gating of `selectedBy` and the read-only restriction.

Fixtures audit:

- `tests/simpleJsonFile.jmap`, `tests/selectedByInheritance.jmap` and
  `tests/muxedPolled.jmap` must comply with the read-only constraint (all entries
  default to `Access::READ_WRITE` when `access` is unset, `JsonMapFileParser.cc:286`);
  non-compliant examples get explicit `access: "RO"` or are marked as legacy.

## Test plan

Focus: **interrupt-triggered registers with `selectedBy`**, plus polled regression.

- New fixture `tests/selectedByInterrupt.jmap`: an interrupt-triggered, double-buffered
  (or plain pull) data register `DAQ.DATA` carrying `selectedBy {register: DAQ.MUX_SEL,
  value: 1}`, a `MUX_SEL` control register, and a second alternative `DAQ.DATA_ALT`
  (`value: 2`) sharing the same buffer base, mirroring the doc's `SINGLE_MUXED`/`_ALT`
  example at `jmapFormat.dox:406-413`. Reuse the existing double-buffer interrupt test
  infrastructure (CR-001/CR-002) and the simulated-firmware pattern to drive selector
  changes and buffer fills.
- Parser tests (`testJsonMapFileParser.cpp`):
  - **PR1** `selectedBy` on a register with `Access::READ_WRITE` throws `logic_error`.
  - **PR2** `selectedBy` on a register with `Access::WRITE_ONLY` throws `logic_error`.
  - **PR3** `selectedBy` on a register with `Access::READ_ONLY` is accepted.
  - **PR4** `selectedBy` on a register with `Access::INTERRUPT` is accepted.
  - **PR5** a module/parent declaring `selectedBy` throws `logic_error` if any descendant
    register is writable (error points to the parent, not the child).
- Catalogue tests (`testJsonMapFileParser.cpp`):
  - **C1** `getSelectedBy("DAQ.DATA")` returns `{MQ.MUX_SEL, 1}`.
  - **C2** a register with no `selectedBy` returns `std::nullopt` (regression).
- Polled scalar/1D gating (`testNumericAddressedBackendRegisterAccessor.cpp`):
  - **P1** selector matches → `read()` returns `DataValidity::ok` and the real data.
  - **P2** selector differs → `read()` returns `DataValidity::faulty` (payload is
    unspecified, per *Payload when unselected*; no assertion on the returned bytes).
  - **P3** switching the selector to the other value then reading yields that
    alternative's data (`SINGLE_MUXED` vs `SINGLE_MUXED_ALT` at the same address).
- Polled 2D gating (`testMultiplexedDataAccesor.cpp`):
  - **PD1** 2D register whose channel selection is met → that channel valid.
  - **PD2** a channel whose own selector is not met → that channel marked
    `DataValidity::faulty` (payload unspecified) while the selected channels of the same
    read remain valid (per-channel gate).
- Interrupt-driven gating (**primary focus**, `testAsyncRead.cpp`):
  - **I1** `wait_for_new_data` on `DAQ.DATA` (sel=1): with `MUX_SEL==1`, each interrupt
    returns the freshly filled buffer — same as CR-001, but now genuinely
    selection-gated.
  - **I2** selector set to the *other* alternative: the consumer does **not** wake with
    the inactive alternative's data (stays pending / no new version published).
  - **I3** selector switched 1→2 while pending: consumer wakes on the next interrupt
    only *after* `MUX_SEL==1` is restored, and never observes data tagged with the wrong
    value.
  - **I4** initial value on `activateAsyncRead` while unselected → delivered immediately
    marked `DataValidity::faulty`; becomes valid once the selector matches and new data
    arrives.
  - **I5** double-buffered + muxed: drive several buffer swaps alternating the selector,
    assert every delivered version corresponds to a buffer filled *while selected*
    (combines CR-001 with `selectedBy`).
- Regression:
  - **R1** full `ctest` of the double-buffering (+CR-001/CR-002/CR-004) suites.
  - **R2** full `ctest` of the async + multiplexed suites.
  - **R3** no behavioural change for registers without `selectedBy`.
- Build & run:
  ```sh
  cmake -S . -B build && cmake --build build
  ctest --test-dir build --output-on-failure -R "AsyncRead|DoubleBuffer|MultiplexedDataAccessor|JsonMapFileParser|NumericAddressedBackendRegisterAccessor"
  ```

## Alternatives considered

- **Domain-level gate in `prepareIntermediateBuffers()`**, signalling "unselected" by
  returning `false`: rejected. A `false` return is documented
  (`AsyncAccessorManager.h:149-156`) to mean the read failed and the implementation
  *must* call `setException()`; "unselected" is not an error. The per-variable gate in
  `fillSendBuffer()` reuses the existing stale-version machinery instead (unchanged
  version + faulty) and leaves `prepareIntermediateBuffers()` untouched.
- **Domain→"primary register" `selectedBy` lookup**
  (`getSelectorRegisterPath(qualifiedAsyncDomainId)`): rejected. It is ambiguous exactly
  in the main use case, where the domain *is* a single muxed 2D register (`DAQ.FD`) with
  per-channel `selectedBy` values — there is no single domain selector value. Replaced by
  the per-register `getSelectedBy(path)`, with a full 2D register returning `nullopt` and
  per-channel validity governing.
- **Gating only the interrupt path**, leaving polled accessors ungated: rejected. Polled
  consumers of a shared muxed address need the same validity, so the gate lives in the
  synchronous accessors (§2D/scalar/1D/double-buffer above) and the async path adds only
  the wake/version suppression.
- **Overwriting the buffer with the inactive alternative** while unselected: rejected.
  The inactive layout is not written into the buffer and the payload is marked faulty, so
  a consumer is never misled into treating the inactive alternative as valid (it must gate
  on `DataValidity::faulty`; the exact payload is unspecified per *Payload when
  unselected* above).
- **Selector-chain recursion** (a selector register that is itself conditionally
  selected) and **relative `regPath` resolution**: deferred, out of scope for this change.
- **Enforcing selection in other backends** (LMap, PCIe): out of scope; this change is
  scoped to the `NumericAddressedBackend` (the base-catalogue `getSelectedBy` default
  keeps them unaffected).
