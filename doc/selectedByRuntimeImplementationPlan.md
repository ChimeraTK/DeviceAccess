# Runtime `selectedBy` Enforcement in the `NumericAddressedBackend`

Synopsis: The map-file parsing of `selectedBy` is already complete — every register
and channel carries an effective `NumericAddressedRegisterInfo::ChannelInfo::selectedBy`
(a `regPath` + `val`), propagated recursively from modules/register-modules and defaulted
onto 2D channels — but the value is currently dead metadata. This change adds the
**runtime enforcement** so that a register declared `selectedBy` is only reported as
*active* while the selected register equals the configured value, for both polled
accessors and interrupt-driven (`wait_for_new_data`) accessors. Reads are gated per
register/channel-set, so mutually exclusive mux alternatives (e.g. DAQ tabs) activate
independently. Two gating granularities exist for a 2D register: a *register-level*
`selectedBy` (declared on the register itself) gates the **whole 2D block** through the
full-2D accessor, while a *per-channel* `selectedBy` gates the named channel slices
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
  register **always performs the physical read** (the address may be shared with an
  alternative; reading is harmless), and the gate is evaluated **after** the read in
  `doPostRead()`. The returned data is marked `DataValidity::faulty` when the selector
  register does not match. The payload is **unspecified** when unselected (see *Payload
  when unselected* below).
- **2D per-channel gate**: each channel of a muxed 2D register is gated by its own
  effective `selectedBy` (its own, falling back to the register-level default / inherited
  value). An inactive channel is marked `DataValidity::faulty` while the selected
  channels of the same read remain valid. Because `doPostRead()` does not overwrite
  inactive channels with the inactive layout, the buffer typically still holds prior
  data, but the inactive channel's payload is also **unspecified** (see *Payload when
  unselected* below). This matches the muxed-layout example where `AmplitudeCh0` (sel 0)
  and `RawCh0` (sel 1) coexist in one register: in a single read only the
  currently-selected channel set is valid.
- **2D register-level gate (whole block)**: a `selectedBy` declared on the 2D register
  itself (as opposed to on individual channels) gates the **whole 2D block**: the
  full-2D accessor is only valid while the selector matches the register-level value — the
  physical read is **always performed**, and the data is marked `DataValidity::faulty` and
  treated as not-new when the gate is closed (evaluated after the read in `doPostRead()`).
  This gate is
  **independent** of the per-channel `selectedBy` the channels may carry: a named channel
  slice gated by its own per-channel value remains valid even if the register-level gate
  is closed (the register-level declaration does **not** override or become the default
  for channels that declare their own per-channel `selectedBy`). Conversely, the full-2D
  read is faulted by the register-level gate even when an individual channel slice is
  per-channel-selected. A register-level `selectedBy` on a 2D register is used **only** as
  the whole-block gate (stored in `NumericAddressedRegisterInfo::registerSelectedBy`, see
  *2D muxed accessor* below); channels without their own declaration still inherit it as
  their per-channel default (via the existing channel-fill path), so for such a register
  the slices and the whole block are gated by the same selector.
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

- New `include/SelectedByDecorator.h` / `src/SelectedByDecorator.cc`: a
  `NDRegisterAccessorDecorator<UserType>` template (the GoF decorator) that wraps the
  gated data accessor and applies the `selectedBy` `DataValidity` policy in
  `doPostRead()`, and (when transfer-skipping is enabled by the interrupt path) skips the
  physical read of the wrapped data accessor in `doReadTransferSynchronously()` when the
  gate is closed.
  Replaces the former concrete `SelectorGate` helper.
- `include/NumericAddressedBackendRegisterAccessor.h` / `src/...cc`: scalar/1D reads are
  wrapped in a `SelectedByDecorator` when the register is declared `selectedBy`.
- `include/NumericAddressedBackendMuxedRegisterAccessor.h` / `src/...cc`: the full-2D
  read does **not** apply per-channel gating — per-channel gating belongs to the named
  channel-slice accessors, which carry their own decorator. The demux simply reports its
  accessor-global validity. A 2D register that declares a **register-level** `selectedBy`
  gets its full-2D accessor additionally wrapped in a `SelectedByDecorator` (the whole-2D
  block gate, see *2D muxed accessor* below).
- `include/NumericAddressedRegisterCatalogue.h` / `src/NumericAddressedRegisterCatalogue.cc`:
  `NumericAddressedRegisterInfo` gains a `registerSelectedBy` member that records a 2D
  register's register-level `selectedBy` (the whole-block gate), distinct from the per-channel
  `selectedBy` each `ChannelInfo` carries; the catalogue `getSelectedBy()` returns it for 2D
  registers (see *Catalogue lookup* below).
- `include/DoubleBufferAccessor.h` / `src/DoubleBufferAccessor.cc`: double-buffer reads
  are wrapped in a `SelectedByDecorator` (the gate also treats the read as not-new when
  unselected).
- `include/async/TriggeredPollDistributor.h` / `src/async/TriggeredPollDistributor.cc`:
  no gating logic of its own — it enables transfer-skipping on each subscription's
  `SelectedByDecorator` (the low-level transfer element), so on an interrupt with the
  gate closed the physical data read is skipped.
- `src/JsonMapFileParser.cc`: enforce the read-only constraint.
- `doc/jmapFormat.dox`: document runtime gating + the read-only restriction.
- `tests/...`: new tests and fixtures (see Test plan), plus an audit of existing jmap
  fixtures (see below).

Selector gate (`SelectedByDecorator<UserType>`):

- A `NDRegisterAccessorDecorator<UserType>` template (the GoF decorator) wrapping the
  gated data accessor. It holds a `ScalarRegisterAccessor<int64_t>` for the selector
  register (owned, self-read on check/doPostRead), the expected value, and the validity
  policy.
- `doPostRead()` applies the polled semantics (mark `DataValidity::faulty` when
  unselected; also treat the read as not-new for the double-buffer case).
- `doReadTransferSynchronously()` applies the interrupt (skip) semantics: when transfer
  skipping is enabled (`setSkipOnUnselected(true)`, set by the async path) and the gate
  is closed, the **physical read of the wrapped data accessor is skipped entirely** (the
  selector is still read so a later selection change is picked up). In the polled path
  skipping is left disabled, so the physical read always happens.
- `check()` / `isSelected()` evaluate the selector state; `isSelected()` reports the most
  recent gate decision without re-reading, used by the async delivery path to suppress the
  wake of an unselected subscription.
- Reuses the existing `ScalarRegisterAccessor` /
  `getSyncRegisterAccessor<int64_t>` so no new I/O primitive is needed.

Scalar / 1D accessor (`NumericAddressedBackendRegisterAccessor`):

- Hold an optional `SelectorGate` built from `registerInfo.channels[0].selectedBy`.
- In `doReadTransferSynchronously()` still always perform the physical read, then in
  `doPostRead()` apply the gate to set `DataValidity` per the polled semantics above.
- No write-path changes are needed (`selectedBy` is restricted to read-only registers).

2D muxed accessor (`NumericAddressedBackendMuxedRegisterAccessor`):

- `doPostRead()` demultiplexes every channel unconditionally and reports its
  accessor-global validity (`DataValidity::ok`). Per-channel `selectedBy` gating is **not**
  applied at the full-register level: it belongs to the named channel-slice accessors,
  which are individually wrapped in a `SelectedByDecorator` (per *Payload when
  unselected* above). The slice accessor's accessor-global validity then equals that
  channel's own selection state.
- **Register-level gate**: when the 2D register declares a *register-level* `selectedBy`
  (recorded in `NumericAddressedRegisterInfo::registerSelectedBy` by the parser, see
  *Parser* below), the full-2D accessor itself is wrapped in a
  `SelectedByDecorator` (`src/NumericAddressedBackend.cc`). This decorator gates the
  *whole 2D block*: the full-2D physical read is **always performed**, and the gate is
  evaluated afterwards in `doPostRead()` — the data is reported `DataValidity::ok` while
  the selector matches the register-level value, and `DataValidity::faulty` (treating the
  read as not-new) while the gate is closed, per the polled semantics. This is independent
  of the per-channel decorators on the channel slices: a slice with its own per-channel
  `selectedBy` is gated solely by that value and is unaffected by the register-level gate
  (see the *2D register-level gate (whole block)* requirement).

Double-buffered accessor (`DoubleBufferAccessor`):

- Double-buffered data registers are the classic interrupt-triggered muxed case (one
  firmware data block, switchable content selected by a control register). The
  `DoubleBufferAccessor` is wrapped in a `SelectedByDecorator`; the decorator decides
  whether the buffer just read is the active one, and if not the read is treated as
  not-new (see the async integration below).

Async interrupt integration (per subscription — main focus):

- The gate lives entirely in the low-level transfer element (the `SelectedByDecorator`),
  not in `TransferGroup` or `TriggeredPollDistributor`. `createAsyncVariable()`
  (`TriggeredPollDistributor.h`) retrieves the synchronous data accessor, which for a
  register declared `selectedBy` already is a `SelectedByDecorator`, and enables
  transfer-skipping on it (`setSkipOnUnselected(true)`). It performs no gate orchestration
  itself.
- On each interrupt, `_transferGroup.read()` calls the decorator's
  `doReadTransferSynchronously()`, which reads the selector *before* the data transfer:
  - **selected** ⇒ the physical read of the data register is forwarded as usual;
  - **unselected** ⇒ the physical read of the data register is **skipped entirely** (the
    expensive, possibly shared/muxed hardware block is not touched); the decorator marks
    the read `DataValidity::faulty` and not-new in `doPostRead()`.
  The selector is read regardless (per-register; not deduplicated across subscriptions) so a
  later selection change is picked up on a subsequent interrupt.
- Delivery: `PolledAsyncVariable::fillSendBuffer()` (`TriggeredPollDistributor.h`) asks the
  decorator `isSelected()`: if the selection is not met (and the initial value has already
  been delivered) it returns `false`, so the consumer does **not wake** with the inactive
  alternative. The initial value is always delivered even while unselected (marked faulty).
- Do **not** signal "unselected" by returning `false` from `prepareIntermediateBuffers()`. Per
  the contract (`AsyncAccessorManager.h:149-156`) a `false` return means the read failed and
  *must* call `setException()`; "unselected" is not an error. `prepareIntermediateBuffers()`
  keeps returning `true`, and its domain-global `_forceFaulty`/version machinery stays
  reserved for genuine DataConsistencyRealm staleness only.
- Note: a **full-register** subscription to a muxed 2D register becomes transfer-skipped/gated
  exactly when the register declares a *register-level* `selectedBy`: `getSelectedBy()`
  then returns `registerSelectedBy` and the whole-2D gate suppresses waking while the
  register-level selector is not met (the register-level value is delivered as
  `DataValidity::faulty` with an unchanged version). A muxed 2D register with **only**
  per-channel `selectedBy` (e.g. `DAQ.FD`) has no register-level gate, hence no wake
  suppression and no per-channel gating at the full-register level; the per-channel
  validity only appears on the named channel-slice accessors (§2D accessor above). This
  addresses the "different channel sets" use case at the slice granularity, while the
  whole-2D gate addresses the "whole block conditional" use case.
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
  *effective* selection from the already-propagated metadata:
  - scalar/1D register (including generated channel slices like `DAQ.FD/AmplitudeCh0`,
    whose `selectedBy` the parser copies at `JsonMapFileParser.cc:464-465`) →
    `channels[0].selectedBy`;
  - full 2D muxed register → `info.registerSelectedBy` (the register-level `selectedBy`
    recorded when the 2D register declares one, gating the whole block). `nullopt` for a
    2D register without a register-level `selectedBy`, in which case there is no single
    gate and the per-channel values govern only inside the accessor;
  - double-buffer `BUF0`/`BUF1` views → they are plain slices of the register they view
    and inherit the same `ChannelInfo` (`JsonMapFileParser.cc:399-411`), so no special
    case.
- This is a pure lookup of metadata the parser already computed — no domain→register
  back-mapping and no `getQualifiedAsyncId` coupling, hence no ambiguity when a domain
  mixes registers with different selections. `NumericAddressedBackend::activateSubscription`
  (`NumericAddressedBackend.cc:267`) is unchanged: the gate is built per subscription
  inside `createAsyncVariable`, which already has the descriptor.

Backend accessor construction (`getSyncRegisterAccessor`):

- A single place to route based on `selectedBy` (`src/NumericAddressedBackend.cc`):
  - scalar/1D with a `selectedBy` → a `NumericAddressedBackendRegisterAccessor` wrapped in
    a `SelectedByDecorator` (self-owned selector read);
  - 2D without a register-level `selectedBy` → `NumericAddressedBackendMuxedRegisterAccessor`
    (no full-register gate; per-channel gating lives on the slice accessors);
  - 2D **with** a register-level `selectedBy` (`info.registerSelectedBy` set) → the same
    muxed full-2D accessor, additionally wrapped in a `SelectedByDecorator` to gate the
    whole 2D block (see *2D muxed accessor* above). Per-channel decorators on the channel
    slices are unaffected;
  - double-buffer → a `DoubleBufferAccessor` wrapped in a `SelectedByDecorator`.
- No structural change to the underlying accessors; the decorator is layered on top.

Parser: register-level `selectedBy` on 2D registers (`JsonMapFileParser`):

- In the 2D (channels-bearing) branch, after folding the register-level/inherited
  `selectedBy` into each channel as its per-channel default (the existing `channel->fill`
  path), the effective *register-level* `selectedBy` (own declaration, or inherited from a
  containing module/register) is additionally recorded in
  `NumericAddressedRegisterInfo::registerSelectedBy`
  (helper `applyRegisterSelectedBy2D`). This member gates the whole 2D block and is what
  `getSelectedBy()` returns for the full-2D accessor and the async subscription. For
  scalar/1D registers the selection is still folded into the single channel (no
  `registerSelectedBy`).



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
  - **C3** a 2D register with a register-level `selectedBy` returns it via `getSelectedBy()`;
    the parser records it in `NumericAddressedRegisterInfo::registerSelectedBy` (own
    declaration on `/REG2D_TOP` → `{/APP/TOP_SEL, 11}`; a 2D register under a selected
    module inherits the module value, e.g. `/INHERIT/CHANB` → `{/APP/OUTPUT_SELECT, 3}`,
    independent of per-channel overrides).
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
  - **PD3** register-level `selectedBy` on a 2D register (`MQ.FD_GATED`, register-level
    `MQ.MUX==2`) gates the *whole* block: the full-2D accessor is `ok` while `MUX==2` and
    `faulty` (with an unchanged version number, i.e. not-new) while the register-level
    selector differs (`testSelectedByRegisterLevelGatesFull2DRead` in
    `testMultiplexedDataAccesor.cpp`).
  - **PD4** independence of the whole-block gate from per-channel gating on the same 2D
    register: a slice with its own per-channel `selectedBy` (`MQ.FD_GATED/B`, `MUX==3`)
    stays `ok` even while the register-level gate closes the whole block (`MUX!=2`), and
    the full-2D accessor stays `faulty` even while such a slice is selected.
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
  *must* call `setException()`; "unselected" is not an error. The per-variable skip in the
  `SelectedByDecorator` instead leaves `prepareIntermediateBuffers()` untouched, and
  `fillSendBuffer()` (which per its contract may suppress delivery by returning `false`)
  drops the delivery when the gate is closed.
- **Domain→"primary register" `selectedBy` lookup**
  (`getSelectorRegisterPath(qualifiedAsyncDomainId)`): rejected. It is ambiguous exactly
  in the main use case, where the domain *is* a single muxed 2D register (`DAQ.FD`) with
  per-channel `selectedBy` values — there is no single domain selector value. Replaced by
  the per-register `getSelectedBy(path)`; a full 2D register returns its register-level
  `registerSelectedBy` (the whole-block gate) when one is declared, otherwise `nullopt`
  with per-channel validity governing.
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
