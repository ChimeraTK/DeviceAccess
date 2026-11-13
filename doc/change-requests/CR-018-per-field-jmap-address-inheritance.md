# CR-018: fix: Per-field inheritance of JMAP addresses

Synopsis: The parser does not implement the per-field address inheritance that
`doc/jmapFormat.dox` and `schemas/jmap.schema.json` promise: a partial `address`
and a partial `secondaryBufferAddress` silently fall back to the member defaults
`IO`/`0` and `DMA`/`0`. Related, an entry that is a register is dropped when it
has `channels` but no `representation`, and a module in the hierarchy breaks the
inheritance chain.

Status: PLANNED

## Requirements

Aspect: per-field inheritance of a partial `address`.

- Current behaviour (bug): a present `address` replaces the inherited address
entirely. Omitted `type`/`channel` silently take the member defaults
`IO`/`0` (`src/JsonMapFileParser.cc:155-167`), so the documented example
(`doc/jmapFormat.dox:242-261`) puts `APP.CONTROL` on BAR 0 instead of BAR 2.
- A present `address` must carry `offset` (`schemas/jmap.schema.json:247`), so
only `type`/`channel` can be omitted there; an omitted `type` inherits from the
nearest ancestor entry that has an address, and with it the `channel`.
- A `type` of its own drops the inherited `channel` instead of taking it over, so
that `channel` is the entry's own value or `0`. A `type` changes what a channel
means, so carrying a channel across a type change makes no sense. The current
code has this outcome as well, by replacing the whole inherited address; the
fix must keep it.
- With no ancestor address, omitted `type`/`channel` still default to `IO`/`0`
(unchanged).

Aspect: register or module, and the inheritance chain.

- An entry with neither `address` nor `representation` nor `channels` is a
module: it yields no register and carries no data of its own
(`doc/jmapFormat.dox:196`).
- Current behaviour (bug): the parser classifies by the `representation` member
alone (`src/JsonMapFileParser.cc:542`). An entry with `channels`, but neither
an own address nor a `representation`, is dropped without a register and
without an error, although it is a register (`schemas/jmap.schema.json:97`).
- An entry with `channels` is always a register, whatever resolves its address
(`schemas/jmap.schema.json:144`): `channels` alone makes it one, and its
`address` is its own or inherited like for any other register.
- The channel entries below `channels` never carry an address of their own and
always take the resolved address of their direct 2D parent, which is always a
register; the nested bit-field `children` of a channel do the same
(`schemas/jmap.schema.json:305-360`).
- Current behaviour (bug): a module breaks the inheritance chain. The inherited
address of a child is re-resolved with
`catalogue.getBackendRegister(parentName)` (`:544`), so `APP.SUB.REG` with
`SUB` a module fails with `Register '/APP/SUB' does not exist`; two modules in
between fail alike.
- A module passes the inherited address on: a child inherits from the nearest
ancestor that has an address, through any number of modules.
- Current behaviour (bug): with no address anywhere in the ancestry, the same
lookup fails with the misleading `Register '/TOP' does not exist` instead of
`Address not set but representation given` (`:333-335`), which is the intended
error.

Aspect: inheritance of the register properties.

- Current behaviour (bug): `access` is inherited only where an entry omits its
  whole `address` (`src/JsonMapFileParser.cc:346-353`), and
  `triggeredByInterrupt` is not inherited at all, so an address-less child of an
  interrupt register becomes an `INTERRUPT` register with no interrupt source.
- `access` is inherited from the nearest ancestor register whenever the entry
  gives no `access` of its own, with an `address` or without.
- `triggeredByInterrupt` is inherited together with the `access` it implies,
  unless the entry gives an `access` of its own; the two remain mutually
  exclusive on a single entry (`schemas/jmap.schema.json:178`). Giving `access`
  is thus the way to keep a child a plain register.
- `numberOfElements` and `bytesPerElement` are inherited only where the entry
  omits its whole `address`, since they describe the sample the entry is cut
  from. An own `bytesPerElement` overrides the sample width in both cases.
- `representation`, `engineeringUnit` and `description` are never inherited.

Aspect: `secondaryBufferAddress`.

- Current behaviour (bug): its omitted fields silently take the member defaults
`DMA`/`0` (`src/JsonMapFileParser.cc:120-127`) instead of the register's
address, contradicting `doc/jmapFormat.dox:420` and the schema `$ref` to
`address` (`schemas/jmap.schema.json:371-373`). The computed secondary BAR then
differs from the primary one and the parse fails, e.g. a register on channel 18
with `secondaryBufferAddress: { "offset": ... }` reports
`primary BAR 18, secondary BAR 13`.
- An omitted `type` inherits the register's resolved `type` and `channel` as a
pair; a `type` of its own leaves an omitted `channel` at `0`.
- Its `offset` is its own and required, like for a primary `address`: the schema
`$ref` to `address` requires `offset` (`schemas/jmap.schema.json:371-373`), so
a present `secondaryBufferAddress` without `offset` is rejected instead of
inheriting the register's `offset`.
- A `type`/`channel` of its own that resolves to a different BAR than the primary
address is still rejected.

Aspect: documentation and schema.

- `doc/jmapFormat.dox:242` and `schemas/jmap.schema.json:227` state the per-field
inheritance of `address`, but leave open whether `offset` may be omitted in a
present address (`:227` says "and/or `offset`", while `:247` requires it).
- Both describe the rule the parser must follow: a present `address` carries
`offset`, inherits an omitted `type` together with its `channel`, and drops the
inherited `channel` where the entry gives its own `type`; only an omitted
`address` inherits `offset` as well. `schemas/jmap.schema.json:227` must state
the `type`/`channel` tie, its "and/or" wording suggests a free choice of
fields.
- `doc/jmapFormat.dox:196` defines a module by its missing `address` alone, which
also covers entries that are registers; it must name all three missing members
instead. `schemas/jmap.schema.json:203`, which already lets a 2D register
inherit its address, is correct and stays as it is; the parser was the side
that deviated.
- Both additionally state that a `secondaryBufferAddress` inherits its omitted
`type`/`channel` from the register's resolved address, with the same
`type`/`channel` tie; its `offset` is its own and required, so a present
`secondaryBufferAddress` without `offset` is rejected like a primary `address`.
- The inheritance example in `doc/jmapFormat.dox:242-261` must stop claiming an
identical outcome to the example above it: its `STATUS` child omits the address
and therefore keeps `numberOfElements: 2` of `APP`.

## Specifications

Affected components: `src/JsonMapFileParser.cc`, `schemas/jmap.schema.json`,
`doc/jmapFormat.dox` (wording only).

Field presence:

- `Address::type`, `Address::channel` and `Address::offset` (and the
`DoubleBufferingInfo::SecondAddress` equivalents) become `std::optional`: an
omitted field deserialises to `std::nullopt`, so a field given in the map is
told apart from an omitted one. The `WITH_DEFAULT` deserialisation keeps that
absence, where today the `IO`/`0` pre-set members survive it, which is what
loses the inheritance.
- The field-presence checks use `has_value()` instead of the
`addressTypeNotSet` and `numeric_limits<size_t>::max()` sentinels:
`resolveAddress` (`:304-331`) and `DoubleBufferingInfo::fill` (`:146-171`).
- A present `address` without `offset` (an unset `std::optional`) is rejected
instead of silently using a sentinel value; the schema already requires
`offset` (`schemas/jmap.schema.json:247`).
- `JsonAddressSpaceEntry::address` becomes `std::optional<Address>`. A present
address with unset fields means "inherit" per the resolution rule below; an
absent address means "inherit the whole ancestor address". This replaces the
current `type == addressTypeNotSet` absence test (`:167`, `:299`, `:339`,
`:444`, `:554`).
- `DoubleBufferingInfo::SecondAddress` uses the same `std::optional` fields for
`type`/`channel`/`offset` instead of the `DMA`/`0`/`0` defaults; a present
`secondaryBufferAddress` without `offset` is rejected like a primary
`address`.

Register or module:

- The module test becomes "no `address`, no `representation`, no `channels`"
instead of the current `representation`-only test (`:542`).
- A module creates no register, but passes the resolved ancestor address on like
a register does; it is no longer a dead end for its children.
- An entry with `channels` always yields a register, even without a
`representation` and without an own `address`; the error
`Address must be set for entries with channels` (`:329-331`) is removed.
- The 2D parent passes its resolved address on to its channel entries and those
to their bit-field `children`; none of them resolves an address of its own.
- An entry with `channels` and no address anywhere in the ancestry fails like
any other address-less register; the error `Address not set but representation
given` (`:333-335`) is reworded so that it also fits an entry that carries
`channels` rather than a `representation`.
- An entry with a `representation` or with `channels` and no address anywhere in
the ancestry keeps the error at `:333-335`, whose wording no longer names only
the `representation` case.

Recursion:

- `addInfos` takes the parent entry instead of a resolved address, and hands
  `this` to its children, so the entry carries the values its children inherit.
  This replaces the `addressSetByParent` flag (`:276`, `:285`, `:319`, `:432`,
  `:448`, `:546`, `:554`) and the `catalogue.getBackendRegister(parentName)`
  lookup (`:544`).
- `JsonAddressSpaceEntry` gains a `derived` member holding the resolved
  `address`, the effective `access` and interrupt sources, the sample width, the
  element pitch, the element count, the effective `selectedBy` and the entry
  that declared it. `derive()` resolves it once per entry from the parent entry
  and the entry's own members and is the only place that carries the inheritance
  rules. `fill` writes `derived` into the register info and keeps no inheritance
  logic of its own (`AncestorRegister`, `:120-128`, disappears).
- An entry with a present `address` resolves `type` from its own value, else
  from the inherited address, else from the `IO` default. `channel` is its own
  value, else the inherited one where the `type` is inherited as well, else `0`.
  `offset` is always its own.
- `access` is the entry's own value, else the parent's.
- `triggeredByInterrupt` is the entry's own value, else inherited together with
  the `access` it implies; specifying `access` suppresses that inheritance.
  Giving both on the same entry stays an error.
- `numberOfElements` and `bytesPerElement` are inherited only where the entry
  omits its whole `address` and gives no `channels`; an own `bytesPerElement`
  then still overrides the sample width. `representation`, `engineeringUnit`
  and `description` are never inherited.
- The resolved address becomes the inherited address of the entry's children, so
  inheritance works through modules and from a 2D register down to its channels
  and their bit fields. A module copies the parent's `derived` unchanged.

Double buffering:

- `DoubleBufferingInfo::fill` resolves the secondary `type`/`channel` against the
register's resolved address by the same rule, so an omitted `type` brings the
register's channel along, before computing the secondary BAR.
- The secondary `offset` is never inherited: a present `secondaryBufferAddress`
must carry it, matching the primary `address` rule and the schema `$ref`.
- The same-BAR check and its error message are unchanged.

### Alternatives considered

- Inheriting the `channel` even where the entry gives its own `type`: rejected,
a channel means something different per `type`, so an inherited channel across
a type change is meaningless.
- Defaulting `secondaryBufferAddress` to `IO`/`0`, as the schema `$ref` to
`address` suggests: rejected, a DMA or non-zero-channel primary address would
then spuriously fail the same-BAR check.
- Dropping `type`/`channel` from `secondaryBufferAddress` entirely, always
taking the primary BAR: rejected, distinct `type`/`channel` pairs may map to
the same BAR and explicit control is kept.
- Inheriting an omitted `secondaryBufferAddress` `offset` from the register:
rejected, the second buffer would share the primary's address and the two
buffers would be indistinguishable.
- Making every entry below an address-bearing ancestor a register, its omitted
`representation` defaulting to 32-bit (`doc/jmapFormat.dox:104`): rejected, it
would add a register for every grouping module, against
`doc/jmapFormat.dox:196` ("A module is not a register and carries no data of
its own").

## Test plan

All test cases go into `tests/executables_src/testJsonMapFileParser.cpp`. Each
case loads `tests/simpleJsonFile.jmap` as the base map, injects its change into
the JSON tree and writes the result to a temporary file, like the existing
selectedBy and map-format-version helpers, so no fault-only fixtures are added.
The shared fixture is used by the backend test binaries as well, which no case
here affects. A rejection case expects a `ChimeraTK::logic_error` and nothing
more; no case asserts an error message.

- The documented inheritance example (`doc/jmapFormat.dox:242-261`): an
address-bearing parent on `IO` channel 2 with one child carrying only a
`representation` and one child whose `address` gives only `offset`; both
children land on the parent's channel (the second currently on channel 0).
- Requirement: per-field inheritance of a partial `address`.
- Necessary: `BSP` in `tests/simpleJsonFile.jmap` already has partial child
addresses (`BSP.VERSION` gives only `offset`, `BSP.SOME_INFO` gives
`channel` 0 and `offset`), but its channel is 0, so an omitted `channel`
yields 0 whether inherited or defaulted. The example's parent on channel 2
is the case that tells the two apart.
- A child `address` giving only `type` below a `DMA` address on channel 5: the
child's own `type` applies and its omitted `channel` falls back to `0`
(unchanged outcome, currently also `0`).
- Requirement: per-field inheritance of a partial `address` (a `type` of its
own drops the inherited `channel`).
- Necessary: the example above covers the inherited `type`/`channel` pair only;
this is the only case with a `type` of the entry's own.
- A child `address` giving only `channel` below a `DMA` address on channel 5: the
inherited `type` applies and the `channel` is the entry's own, not the
inherited 5.
- Requirement: per-field inheritance of a partial `address` (an inherited
`type` brings the channel along).
- Necessary: the other cases have the `channel` omitted; only this one pins
that an inherited `type` leaves an own `channel` in place.
- An entry with `channels`, neither an own `address` nor a `representation`,
below an address-bearing entry: yields a register on that address. Its own
channels resolve to that address, and so does a bit-field `children` entry
nested below one of the channels; the bit-field child is asserted through its
own register, not through the channel slice above it (currently all dropped
silently).
- Requirement: register or module, and the inheritance chain (`channels` alone
makes a register).
- Necessary: every `channels` entry in the existing maps carries its own
`address`; the existing bit-field `children` sit below such channels, so
inheriting a bit-field child through an address-less register is untested.
- An address-bearing entry with a module child holding an address-less child
that carries a `representation`: the address reaches the child through the
module (currently `Register ... does not exist`), and the module itself yields
no register.
- Requirement: register or module, and the inheritance chain.
- Necessary: the existing maps have modules above a register-bearing entry
(`APP`, `MY_INTC`) and a module without any address (`MY_INTC.SUB0`), but no
module between an address-bearing entry and an address-less descendant. One
module level suffices, the pass-through is the recursion itself.
- A present `address`, and likewise a present `secondaryBufferAddress`, without
`offset`: rejected.
- Requirement: per-field inheritance of a partial `address` (`offset` is
always the entry's own, `schemas/jmap.schema.json:247`).
- Necessary: no existing map has an address without `offset`.
- An entry with `channels` and no address anywhere in its ancestry: rejected
(currently dropped silently).
- Requirement: register or module, and the inheritance chain.
- Necessary: this reaches the check that is unreachable today, and dropping the
entry is no error at all today, so the case distinguishes the fix.
- An entry with a `representation` and no address anywhere in its ancestry:
rejected instead of yielding a register.
- Requirement: register or module, and the inheritance chain.
- Necessary: no existing map has a register without any address in its
ancestry. The case cannot tell the current failure apart, a catalogue lookup
of the parent that also throws, because messages are not asserted; it pins
that the entry still fails rather than starting to parse, the wording is left
to the review.
- A double-buffered register whose primary address is `DMA` on a non-zero
channel and whose `secondaryBufferAddress` gives only `offset`: parses, with
the secondary on the primary's channel (currently the spurious "different
BARs" failure). In the same fixture, a `secondaryBufferAddress` giving its own
`type` and `offset` but omitting `channel` does not take the primary's channel
over and is rejected by the same-BAR check.
- Requirement: `secondaryBufferAddress` inherits an omitted `type`/`channel`
from the register's resolved address, with the `type`/`channel` tie.
- Necessary: every `secondaryBufferAddress` in the existing maps gives `type`
explicitly, and the secondary resolution is its own code path
(`DoubleBufferingInfo::fill`), so the tie needs an assertion there too.
- An address-bearing parent with `access: RO` and a child whose own `address`
  carries only `offset`: the child is `READ_ONLY`.
- Requirement: inheritance of the register properties.
- Necessary: the implemented parser applies an inherited `access` only where the
  whole address is omitted, and no fixture combines an own `address` with an
  omitted `access` below a parent whose access is not the default.
- An address-bearing parent with `triggeredByInterrupt` and two address-less
  children carrying a `representation`: the one without `access` is an
  `INTERRUPT` register with the parent's interrupt id, the one giving
  `access: RO` is `READ_ONLY` without an interrupt id.
- Requirement: inheritance of the register properties.
- Necessary: no fixture reaches this rule, since the interrupt registers in the
  maps use `channels` and their bit-field children are a reduced struct that
  never reaches `fill`. The interrupt id is what the implemented parser leaves
  empty, so the case distinguishes the fix.
- Not repeated, already covered today: an `address` giving only `offset` with no
address-bearing ancestor defaults to `IO`/`0`
(`tests/selectedByInheritance.jmap`, e.g. `APP.OUTPUT_SELECT`, below the
address-less module `APP`); a secondary on the same BAR is accepted
(`DAQ.CTRL` in `TestGoodMapFileParse`) and a different BAR is rejected
(`TestDoubleBufferDiffBarThrows`). Those fixtures and tests stay untouched.

## Deferred issue

- DI-1 [DONE] The plan cites the schema reference rule at schemas/jmap.schema.json:178 twice, but line 178 is the access/triggeredByInterrupt exclusivity rule. The channels-inherit-address text is at :144 and :203; correct both citations.
- DI-2 [DONE] The test plan's first case is labelled 'Necessary' on the claim that no existing map has a partial child address. This is factually wrong: tests/simpleJsonFile.jmap already has BSP, an address-bearing parent with partial child addresses BSP.VERSION ({offset 4}) and BSP.SOME_INFO ({channel 0, offset 8}). Re-state the motivation for the case rather than asserting non-existence.
- DI-3 [DONE] The test plan's 'Not repeated' bullet mis-attributes coverage: MY_INTC is defined in tests/simpleJsonFile.jmap with an explicit channel 4, not in tests/selectedByInheritance.jmap, and it does not give only offset. Correct the fixture and field attribution.
- DI-4 [DONE] secondaryBufferAddress/offset inconsistency: the requirements demand that both the format documentation and the specification state a secondaryBufferAddress inherits its omitted fields, but the specification keeps SecondAddress::offset at its default 0 while simultaneously rejecting a present address without offset. The schema types both primary and secondary addresses as #/definitions/address with offset required, so the rejection rule and the inheritance claim must be reco...
- DI-5 [DONE] Implementation defect in Address resolution: an address that gives its own 'channel' but omits 'type' does not keep the own channel. Specification says 'channel is its own value, else the inherited one where the type is inherited as well, else 0', and the test plan's own-channel case expects the own channel to win. resolveAddress (src/JsonMapFileParser.cc:306-320) unconditionally overwrites resolved.channel with the inherited channel whenever the type is inherited, because A...
- DI-6 [DONE] Implementation defect in secondaryBufferAddress offset rejection: a present secondaryBufferAddress without 'offset' must be rejected with ChimeraTK::logic_error like a primary 'address', but DoubleBufferingInfo::fill (src/JsonMapFileParser.cc:153) only asserts (assert(secondaryBufferAddress.offset.v != std::numeric_limits<size_t>::max())). In a debug build the assert aborts (SIGABRT) instead of throwing, so TestSecondaryBufferAddressWithoutOffsetRejected fails and aborts the...
- DI-7 [DONE] tests/executables_src/testJsonMapFileParser.cpp (TestChannelsEntryInheritsAddressBecomesRegister): the second assertion block fetches '/APP/CHANS_PARENT/CHANS_CHILD/B', which is the channel slice named B, but its comment claims 'A bit-field child of a channel resolves to the same address as well'. Channel B is declared as a plain channel ({bytesPerElement 4, offset 0, representation {type fixedPoint, width 4}}) with no 'children', so no bit-field child (a 'children' entry belo...
- DI-8 [DONE] The user requests replacing the 'unset' sentinels with std::optional: Address::type/channel/offset and DoubleBufferingInfo::SecondAddress::type/channel/offset currently use addressTypeNotSet / std::numeric_limits<size_t>::max() to mark an omitted (inherited) field (src/JsonMapFileParser.cc:134-136, :179-181). They should become std::optional, with an omitted JSON field deserialising to std::nullopt and resolveAddress (:304-331) and DoubleBufferingInfo::fill (:146-171) testin...

- DI-9 [OPEN] Mechanism (Specifications, Recursion): the implemented parser keeps
  the inherited state in `AncestorRegister` and passes it as a second argument
  next to the resolved address (`src/JsonMapFileParser.cc:120-128`, `:493`,
  `:511`, `:518`, `:521`, `:617`). It becomes a `derived` member of
  `JsonAddressSpaceEntry` resolved by `derive()`; `addInfos` takes the parent
  entry and hands `this` down; `fill` (`:332-440`) drops `resolved`, `ancestor`
  and both `ownsAddress` branches; `applyRegisterSelectedBy` (`:442-452`) folds
  into `fill`; the child loop (`:616`) and the top-level loop (`:724`) iterate
  non-const. Reasons and rejected alternatives are in the temporary section.
- DI-10 [OPEN] `access` inheritance: implemented only for an entry that omits its
  whole address (`:346-353`); it must apply whenever the entry gives no `access`,
  with an own `address` included. Covered by the new test plan case for `access`
  inheritance.
- DI-11 [OPEN] `triggeredByInterrupt` inheritance: not inherited today, so an
  address-less child of an interrupt register becomes `Access::INTERRUPT` with an
  empty `interruptId` (`:342-362`). It must inherit the parent's sources together
  with that access unless the entry gives its own `access`. Covered by the new
  test plan case for interrupt inheritance; no fixture reaches the rule.
- DI-12 [OPEN] Sample geometry: the behaviour stays as implemented (`:371-383`,
  inherited iff the whole address is omitted and the entry gives no `channels`,
  an own `bytesPerElement` overriding the sample width). Only where the rule is
  expressed changes, `derive()` instead of `fill`. No fixture or test change.
- DI-13 [OPEN] `doc/jmapFormat.dox:242-261`: the example claims an identical
  outcome to the one above it, but its `STATUS` child omits the address and the
  implemented parser inherits `numberOfElements: 2` of `APP` for it, so the
  claim is wrong. The example or the claim must be corrected.
- DI-14 [OPEN] Remove the temporary section once DI-9 to DI-13 are done.

## Temporary section: simplifying the inheritance mechanism

Added during the final review on 2026-11-13 to record the discussion. It is not
part of the change yet; it goes away once the open points are settled.

### What the current mechanism does

- `AncestorRegister` (`src/JsonMapFileParser.cc:120-128`) carries
  `registerAccess`, `elementPitchBits` and `nElements` of the nearest ancestor
  register, and `fill` receives it next to the resolved address (`:332-334`).
- It is read only where an entry omits its whole `address` (`:346-353`,
  `:377-383`). `resolveAddress` (`:308-330`) never looks at it, so address
  inheritance and property inheritance are already independent.
- On master (`84fcd860`) the whole parent register was copied
  (`catalogue.getBackendRegister`, `:544`) and only name and representation
  were overwritten. Every other member, `interruptId` included, came along by
  accident, and the lookup failed whenever a module sat in between.

### Proposed rule

Each property has one rule of its own. A property of the register's role follows
the ancestor whether or not the entry has an `address`; a property of the sample
geometry follows the `address`.

- `access`: inherited from the nearest ancestor register when the entry gives
  no `access`.
- `triggeredByInterrupt`: inherited together with the `access` it implies, but
  only when the entry gives no `access`; the two are mutually exclusive
  (`schemas/jmap.schema.json:178`, `src/JsonMapFileParser.cc:356-359`). Giving
  `access` (`RO` for instance) switches the inheritance off, which is the way
  out for a child that must not become an interrupt register.
- `numberOfElements` and `bytesPerElement`: inherited from the nearest ancestor
  register only where the entry omits its whole `address`; an entry with an
  `address` of its own states them itself. These two are the sample geometry of
  a register, not a property of the register's role, so they follow the address.
  See the decisions at the end.
- `representation`, `engineeringUnit`, `description`: always the entry's own.
- Address fields: as specified above (`type`/`channel` per field, `offset` own
  and required, a wholly omitted `address` inherits the offset as well).
- `channels` and `children` stay mutually exclusive.

### Comparison: master against the proposal

The table states behaviour, not how it is achieved: "yes" or "no" for an entry
that is a register. "Yes" means the entry inherits the field whenever it does
not set it itself; a condition stated here is exhaustive. Columns are master
(`84fcd860`) and the proposal.

| Field | Master | Proposal |
| --- | --- | --- |
| `address` as a whole | yes | yes |
| `type` | no, always `IO` | yes; `IO` without an ancestor |
| `channel` | no, always `0` | yes, if `type` omitted too |
| `offset` | yes, if `address` omitted | yes, if `address` omitted |
| `access` | yes, if `address` omitted | yes |
| `triggeredByInterrupt` | yes, if `address` omitted | yes, unless `access` |
| `numberOfElements` | yes, if `address` omitted | yes, if `address` omitted |
| `bytesPerElement` | yes, if `address` omitted | yes, if `address` omitted |
| `pitch` | no | no |
| `representation` | no | no |
| `engineeringUnit` | no | no |
| `description` | no | no |
| `selectedBy` | yes | yes |
| `doubleBuffering` | no | no |
| `secondaryBufferAddress` | no | no |

The current branch differs from master in the address fields (`type`, `channel`,
`offset`, and the secondary address), which is this change request. Of the
property rows, `access` changes (see below) and `triggeredByInterrupt` gets
back what the branch dropped: an address-less child of an interrupt register
today inherits `Access::INTERRUPT` but keeps an empty `interruptId`, and the
proposal repairs that. All other property rows, `numberOfElements` and
`bytesPerElement` included, stay as they are in master.

Notes on the rows that change behaviour:

- `access`: master inherited it only where an entry omitted its whole address.
  An entry with an own address and no `access` was `READ_WRITE` on master and
  stays so on the branch; the proposal gives it the ancestor's `access`. No
  fixture has that combination (checked for every `tests/*.jmap`), so no test
  is affected.
- `triggeredByInterrupt`: no fixture reaches this either; interrupt registers
  in the maps use `channels`, and their bit-field children are a separate,
  reduced struct (`ChannelChild`, `:275-281`) that never enters `addInfos`.
- `bytesPerElement`, `numberOfElements`: no row changes behaviour here. They are
  different things: `bytesPerElement` is the size of one sample (it fixes the
  raw data type and, without `pitch`, the stride between samples),
  `numberOfElements` is how many samples the register holds. A bit field is cut
  out of one sample of its ancestor, so both decide its geometry: the sample
  size bounds `bitShift + width`, the count says how many samples it spans.
  Both members exist on master as well (`:117-118`). Because they describe the
  sample and not the register's role, they are inherited exactly where master
  inherited them: only by an entry that omits its whole `address`.
- `offset`: master accepted an `address` without `offset` and used a
  meaningless address; requiring the `offset` is already part of this change
  request.
- `secondaryBufferAddress`: master assumed `DMA`, channel 0 and offset 0 for
  omitted secondary fields; requiring the `offset` is already part of this
  change request.

### Impact on the fixtures

Only entries that reach `addInfos` without an own address inherit properties.
In `tests/*.jmap` these are the eleven bit-field `children`:

- `tests/simpleJsonFile.jmap`: `ProbeLimiter`, `ExternalInterlock` and
  `ErrorCounter` below `APP/STATUS` and below `APP/LARGE_STATUS`.
- `tests/unifiedTest.jmap`: `BitRangeLower`, `BitRangeMiddle`,
  `BitRangeUpper` below `WORD_FIRMWARE`, and `ArrayBitRangeLow`,
  `ArrayBitRangeHigh` below `ARRAY_REG`.

`access`, `triggeredByInterrupt`: no fixture depends on either row.

`numberOfElements`: only `ARRAY_REG` sets it (3) for a bit-field parent, and
`testNumericAddressedBackendUnified.cpp:1084-1123` reads both bit ranges as
three-element arrays. The two children omit their address and inherit the
count, so nothing changes.

`bytesPerElement`: only `APP/LARGE_STATUS` sets it (8) for a bit-field parent,
and `testJsonMapFileParser.cpp:236,244,254,261,271,278` asserts its bit fields at
`elementPitchBits == 64` and `rawType == int64`. The three children omit their
address and inherit the sample size, so nothing changes.

No fixture has to be touched. The documented inheritance example
(`doc/jmapFormat.dox:242-261`) does need attention: `APP` carries
`numberOfElements: 2` and its `STATUS` child omits the address, so `STATUS`
holds two elements even though the text claims an identical outcome to the
example above, where `STATUS` has one element. The example and that claim are
to be corrected when the format documentation is updated.

### Mechanism: the entry carries its derived values

Agreed on 2026-11-13 for the implementation phase. It replaces
`AncestorRegister` (`src/JsonMapFileParser.cc:120-128`).

- `JsonAddressSpaceEntry` gains a member `derived` holding what the entry
  effectively is: the resolved `address`, the effective `access`, the effective
  interrupt sources, the sample width, the element pitch, the element count, the
  effective `selectedBy` and the entry that declared it.
- A new `derive(parent, thisPath)` resolves that member once per entry from the
  parent entry and the entry's own members. It becomes the only place that holds
  the inheritance rules of this section.
- `addInfos` takes the parent entry (`const JsonAddressSpaceEntry*`, null for
  the entries of the top-level `addressSpace`) and hands `this` to its children.
  A module copies the parent's `derived` unchanged, a register derives its own
  values from it.
- `fill` loses both inheritance parameters and both `ownsAddress` branches and
  only writes `derived` into the register info. Its single caller is `addInfos`
  (`:518`), so no interface outside the parser is affected.
- `resolveAddress` stays as it is and is called from `derive`, which takes the
  inherited address from `parent->derived.address`.

Why not carry the filled `NumericAddressedRegisterInfo` instead:

- It cannot replace the inherited address: `bar` folds type and channel into
  one number (`:192`), so IO channel 13 and DMA channel 0 both give `bar == 13`
  and a child that gives its own `channel` and omits `type` could not compute
  its `bar`.
- A reference to it would point at a local of the parent's `addInfos` (`:516`)
  and dangle once that call returns; the entry outlives the parse.
- The entry that declared the effective selector (`selectedBySource`, `:505`)
  is not part of the register info, but the error at `:434-436` names it.

Trade-off accepted: the entry is no longer a pure image of the JSON, and the
inheritable set is no longer enforced by a signature, so a child must read
`parent->derived` and nothing else of its parent. This goes in the direction of
the planned long-term change, in which the JSON structure becomes identical to
the info object and parsing writes directly into the catalogue.

### Decisions taken

Settled on 2026-11-13, so the section records no open points.

- Sample geometry: `numberOfElements` and `bytesPerElement` are inherited if
  and only if the entry omits its whole `address`. The earlier idea of
  stating the values on every bit-field child is dropped.
- Consequences: the sample geometry needs no carrier of its own, `derived`
  holds it together with the access, the interrupt sources and the selector. No
  fixture and no test changes. The example in `doc/jmapFormat.dox` is corrected
  separately.
