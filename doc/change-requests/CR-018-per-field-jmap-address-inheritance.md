# CR-018: per-field inheritance of JMAP addresses

Synopsis: The parser must implement the per-field address and register-property
inheritance that `doc/jmapFormat.dox` and `schemas/jmap.schema.json` describe.
Both documents must state that rule precisely.

Status: PLANNED

## Requirements

Aspect: per-field inheritance of a partial `address`.

- A present `address` carries its own `offset` (`schemas/jmap.schema.json:247`);
only `type` and `channel` may be omitted.
- An omitted `type` inherits from the nearest ancestor entry that has an
address, together with its `channel`.
- A `type` of its own drops the inherited `channel`, so an omitted `channel` is
then `0`. A channel means something different per `type`, so it is not carried
across a type change.
- With no ancestor address, an omitted `type`/`channel` defaults to `IO`/`0`.
- A wholly omitted `address` inherits the ancestor's `offset` as well.
- A `secondaryBufferAddress` follows the same rule for its `type`/`channel`; its
`offset` is its own and required, so a `doubleBuffering` object without a
`secondaryBufferAddress` or with one without `offset` is rejected like a
primary `address`.
- A secondary `type`/`channel` resolving to a BAR other than the primary one is
rejected.

Aspect: register or module, and the inheritance chain.

- A module groups nested entries and carries no data of its own: it has no
`address`, no `representation` and no `channels` (`doc/jmapFormat.dox:196`).
- An entry with `channels` is always a register, whatever resolves its address
(`schemas/jmap.schema.json:144`), even without a `representation` and without
an own `address`.
- An entry with a non-void `representation` or with `channels` and no address
anywhere in the ancestry is rejected (`Address not set but representation or
channels given`); a void-typed entry requires no address.
- Inheritance works through any number of modules: a child inherits from the
nearest ancestor that has an address.
- The channel entries below `channels` never carry an address of their own and
take the resolved address of their direct 2D parent, which is always a
register; the nested bit-field `children` of a channel do the same
(`schemas/jmap.schema.json:305-360`).

Aspect: inheritance of the register properties.

- `access` is inherited from the nearest ancestor register whenever the entry
gives no `access` of its own, with an `address` or without.
- `triggeredByInterrupt` is inherited together with the `access` it implies,
unless the entry gives an `access` of its own; the two remain mutually
exclusive on a single entry (`schemas/jmap.schema.json:178`). Giving `access`
is thus the way to keep a child a plain register.
- `numberOfElements` and `bytesPerElement` are inherited only where the entry
omits its whole `address` and gives no `channels`, since they describe the
sample the entry is cut from. An own `bytesPerElement` overrides the sample
width.
- `representation`, `engineeringUnit` and `description` are never inherited.

Aspect: documentation and schema.

- `doc/jmapFormat.dox` and `schemas/jmap.schema.json` state the rules above: the
required `offset` of a present address, the `type`/`channel` tie, the module
definition, the secondary-address inheritance, and the inheritance of
`access`, `triggeredByInterrupt` and the sample geometry. The "and/or
`offset`" wording of `schemas/jmap.schema.json:227` must go, it suggests a
free choice of fields.
- The inheritance example (`doc/jmapFormat.dox:242-261`) must not claim an
identical outcome to the example above it: its `STATUS` child omits the
address and therefore inherits `numberOfElements: 2` of `APP`.

## Specifications

### JsonAddressSpaceEntry (modified) in src/JsonMapFileParser.cc (modified)

- `std::optional<Address> address`, replacing the absent-address test on the
`addressTypeNotSet` type sentinel.
- absent: inherit the whole ancestor address, `offset` included.
- present: `type`/`channel` are inherited per field, `offset` is the entry's
own.
- `std::optional<DoubleBufferingInfo> doubleBuffering` and
`std::optional<SelectedBy> selectedBy` as they are.
- `bool isRegister() const` (new): an entry with an `address`, a
`representation` or `channels` is a register, any other entry is a module.
Used by `derive()` and `addInfos` alike.
- `struct Derived` (new): the values a child of the entry inherits.
- `std::optional<Address> address`: the address an address-less child
inherits. A module and a VOID entry carry the parent's, so it is not
necessarily the entry's own address.
- `NumericAddressedRegisterInfo::Access access` and
`std::vector<size_t> interruptId`, the effective access and its interrupt
sources.
- `size_t sampleWidthBytes`, `size_t elementPitchBits` and
`size_t nElements`. An entry with its own `address` takes
`elementPitchBits` `sampleWidthBytes * 8` for a 1D entry and `pitch * 8` for
a `channels` entry; an address-less entry without `channels` inherits the
parent's `elementPitchBits`. The width defaults to 4 bytes where
`bytesPerElement` is unset.
- `std::optional<SelectedBy> selectedBy` and `std::string
selectedByDeclaredBy`, the effective selector and the dotted path of the
entry that declared it, which the error message names.
- `std::optional<Derived> derived` (new member), written once per entry by
`derive()`.
- It holds the inheritable values only: a child reads `parent->derived` and
nothing else of its parent.
- The entry stays the image of the JSON, `derived` is its resolved view.
- `void derive(const JsonAddressSpaceEntry* parent, const RegisterPath&
pathName)` (new, non-const).
- Writes `derived`: the only place holding the inheritance rules of the
register properties. `parent` is null for the entries of the top-level
`addressSpace`.
- `address`: `resolveAddress(pathName, parent != nullptr ?
parent->derived.address : std::nullopt)`.
- With a null `parent` the entry starts from the defaults: `READ_WRITE`
access, no `interruptId`, `sampleWidthBytes` 4 or its own
`bytesPerElement`, `elementPitchBits` `sampleWidthBytes * 8`, `nElements`
its own `numberOfElements` and no `selectedBy`.
- A module takes `address`, `access`, `interruptId`, `sampleWidthBytes`,
`elementPitchBits` and `nElements` from `parent->derived` unchanged; its
own `access`, `triggeredByInterrupt`, `numberOfElements` and
`bytesPerElement` are ignored, since a module is no register.
- `access`: the entry's own value; else `INTERRUPT` where the entry gives its
own `triggeredByInterrupt`; else the parent's.
- `interruptId`: the entry's own `triggeredByInterrupt`, else the parent's
together with the `access` it implies; an own `access` suppresses that
inheritance. An own `access` next to an own `triggeredByInterrupt` stays
rejected (`:356-359`).
- `sampleWidthBytes`, `elementPitchBits` and `nElements`: the entry's own
values where it gives an `address` or `channels`, else the parent's. An own
`bytesPerElement` overrides the inherited sample width. `sampleWidthBytes`
is the entry's `bytesPerElement`, 4 where unset; a non-`channels` child of
a 2D register inherits the ancestor's `elementPitchBits / 8` as its sample
width, as today.
- `selectedBy`: the entry's own value, else the parent's, for a module as
well; `selectedByDeclaredBy` is `pathName` where the entry declares it, else
the parent's.
- `representation`, `engineeringUnit` and `description` are never taken from
the parent.
- `std::optional<Address> resolveAddress(const RegisterPath& pathName, const
std::optional<Address>& inheritedAddress) const` (new).
- A present `address` without `offset` is rejected with `logic_error`.
- `type`/`channel` come from `Address::resolveTypeAndChannel`.
- The result becomes the inherited address of the entry's children.
- `void fill(NumericAddressedRegisterInfo& info, const RegisterPath& pathName)
const` (modified): writes `derived` and the non-inheritable members into the
register info and holds no inheritance logic.
- Asserts that `derived` has a value, i.e. that `derive()` ran on the entry.
- Loses the `resolved` and `ancestor` arguments and both `ownsAddress`
branches.
- Takes over `applyRegisterSelectedBy` (`:444`), which disappears.
- An entry with a `representation` or with `channels` and no
`derived.address` throws `Address not set but representation or channels
given`.
- A VOID-typed entry writes no geometry and `nElements` 0 instead of
`derived`, and keeps requiring an own `triggeredByInterrupt` and no own
`address` (`:397-412`). Its test is on the own `address`, not on
`derived.address`, which it inherits like any other entry.
- `void addInfos(NumericAddressedRegisterCatalogue& catalogue, const std::string&
name, const RegisterPath& parentName, const JsonAddressSpaceEntry* parent)`
(modified, non-const).
- Takes the parent entry instead of the resolved address and the ancestor
register info, derives `this` once and hands `this` to its children.
- The selector comes from `derived`, so it needs no argument of its own.
- Creates a register for `isRegister()` entries only.
- Hands the children a path carrying the dotted alt separator, which `fill`
expects as given.
- Channel and bit-field slices keep taking the resolved address and geometry
of their 2D parent (`:527-593`), unchanged.
- `struct AncestorRegister` and `OptionalAncestor` (removed).

### JsonAddressSpaceEntry::Address (modified) in src/JsonMapFileParser.cc (modified)

- `std::optional<AddressType> type`, `std::optional<size_t> channel` and
`std::optional<HexValue> offset`.
- An omitted field deserialises to `std::nullopt`, so a field given in the map
is told apart from an omitted one. `WITH_DEFAULT` keeps that absence.
- `fill` asserts that all three are resolved.
- `static std::pair<AddressType, size_t> resolveTypeAndChannel(const
std::optional<AddressType>& ownType, const std::optional<size_t>& ownChannel,
const std::optional<Address>& inherited)` (new): the single copy of the
`type`/`channel` tie.
- An own `type` wins; an omitted `type` takes the inherited `type` together
with its `channel`; with no inherited address it becomes `IO` and the
`channel` `0`.
- An own `channel` always wins.
- `addressTypeNotSet` and the `numeric_limits<size_t>::max()` sentinel (both
removed), which marked an omitted field.

### JsonAddressSpaceEntry::DoubleBufferingInfo (modified) in src/JsonMapFileParser.cc (modified)

- `struct SecondAddress` uses the same `std::optional` fields for
`type`/`channel`/`offset`.
- `void fill(NumericAddressedRegisterInfo& info, const Address& resolvedPrimary)
const` (modified): resolves the secondary `type`/`channel` with
`Address::resolveTypeAndChannel` against the register's resolved address,
before computing the secondary BAR.
- The secondary `offset` is never inherited: a present
`secondaryBufferAddress` without `offset` is rejected with `logic_error`.
- The same-BAR check and its error message are unchanged.

### Format documentation (modified) in doc/jmapFormat.dox (modified)

- `jmap_simple_modules`: a module is defined by the absence of `address`,
`representation` and `channels`; an entry with `channels` is always a
register.
- `jmap_access`: a register that gives no `access` inherits the access of the
nearest ancestor register, together with its `triggeredByInterrupt` unless
the entry gives an `access` of its own.
- `jmap_address_inheritance`: the inheritance works through any number of
modules; `offset` is never inherited; an omitted `type` brings its `channel`
along, an own `type` drops it.
- `numberOfElements` and `bytesPerElement` are inherited only where the whole
`address` is omitted and the entry gives no `channels`, an own
`bytesPerElement` overriding the sample width.
- The register section points to the nearest ancestor instead of a parent
module.
- `jmap_double_buffering`: an omitted secondary `type` inherits the register's
resolved `type` together with its `channel`.
- The inheritance example and its "identical outcome" claim are corrected.

### JSON schema (modified) in schemas/jmap.schema.json (modified)

- `addressSpaceEntry`: the module definition names all three absent members; the
`address` may be inherited.
- `channels`: their presence makes the entry a register, whatever its `address`.
- `address`: `offset` required, without the "and/or" wording; an omitted `type`
inherits `type` and `channel`, an own `type` drops the inherited `channel`.
- `secondaryBufferAddress`: states the same inheritance; the `$ref` to `address`
keeps `offset` required. `doubleBuffering` lists `secondaryBufferAddress` as a
required member, so the schema matches the rejection.
- `access` and `triggeredByInterrupt`: their descriptions state the inheritance;
the `children` description says the nearest ancestor's address instead of the
parent's.

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
- Carrying the filled `NumericAddressedRegisterInfo` of the parent instead of
`Derived`: rejected.
- `bar` folds `type` and `channel` into one number (`:192`), so IO channel 13
and DMA channel 0 both give `bar == 13` and a child that gives its own
`channel` and omits `type` could not compute its `bar`.
- A reference to it would point at a local of the parent's `addInfos` (`:516`)
and dangle once that call returns.
- The entry that declared the effective selector is not part of the register
info, yet the error at `:434-436` names it.
- Stating `numberOfElements` and `bytesPerElement` on every bit-field child
instead of inheriting them: rejected, the sample geometry is the one the entry
is cut from, and every existing fixture relies on inheriting it.

## Test plan

All test cases go into `tests/executables_src/testJsonMapFileParser.cpp`. Each
case loads `tests/simpleJsonFile.jmap` as the base map, injects its change into
the JSON tree and writes the result to a temporary file, like the existing
selectedBy and map-format-version helpers, so no fault-only fixtures are added.
The shared fixture is used by the backend test binaries as well, which no case
here affects. A rejection case expects a `ChimeraTK::logic_error` and nothing
more; no case asserts an error message.

- `tests/executables_src/testJsonMapFileParser.cpp` (modified)
- `TestAddressInheritanceDocumentedExample`: the documented inheritance
example (`doc/jmapFormat.dox:242-261`) - an address-bearing parent on `IO`
channel 2 with one child carrying only a `representation` and one child
whose `address` gives only `offset`; both children land on the parent's
channel.
- Requirement: per-field inheritance of a partial `address`; also pins the
corrected claim of that example, an address-less child inheriting
`numberOfElements: 2` of the parent.
- Necessary: `BSP` in `tests/simpleJsonFile.jmap` already has partial child
addresses (`BSP.VERSION` gives only `offset`, `BSP.SOME_INFO` gives
`channel` 0 and `offset`), but its channel is 0, so an omitted `channel`
yields 0 whether inherited or defaulted. The example's parent on channel 2
is the case that tells the two apart.
- `TestAddressInheritanceOwnTypeDropsChannel`: a child `address` giving only
`type` below a `DMA` address on channel 5; the child's own `type` applies
and its omitted `channel` falls back to `0`.
- Requirement: per-field inheritance of a partial `address` (a `type` of
its own drops the inherited `channel`).
- Necessary: the example above covers the inherited `type`/`channel` pair
only; this is the only case with a `type` of the entry's own.
- `TestAddressInheritanceOwnChannelKept`: a child `address` giving only
`channel` below a `DMA` address on channel 5; the inherited `type` applies
and the `channel` is the entry's own, not the inherited 5.
- Requirement: per-field inheritance of a partial `address` (an inherited
`type` brings the channel along).
- Necessary: the other cases have the `channel` omitted; only this one pins
that an own `channel` wins over the inherited one.
- `TestChannelsEntryInheritsAddressBecomesRegister`: an entry with `channels`,
neither an own `address` nor a `representation`, below an address-bearing
entry; it yields a register on that address, its channel slices resolve to
that address, and so does a bit-field `children` entry nested below one of
the channels.
- Requirement: register or module, and the inheritance chain (`channels`
alone makes a register).
- Necessary: every `channels` entry in the existing maps carries its own
`address`; the existing bit-field `children` sit below such channels, so
an address-less register passing its address to a channel and on to a
bit-field child is untested.
- `TestAddressInheritanceThroughModule`: an address-bearing entry carrying
`access: RO` with a module child holding an address-less child that carries
a `representation`; the address reaches the child through the module, the
child is `READ_ONLY` with the parent's `numberOfElements`, and the module
itself yields no register.
- Requirement: register or module, and the inheritance chain; also the
property pass-through of the module branch.
- Necessary: the existing maps have modules above a register-bearing entry
(`APP`, `MY_INTC`) and a module without any address (`MY_INTC.SUB0`), but
no module between an address-bearing entry and an address-less descendant.
One module level suffices, the pass-through is the recursion itself. This
is the only case reaching the module branch's property pass-through of
`derive()`.
- `TestAddressWithoutOffsetRejected`: a present `address` without `offset` is
rejected.
- Requirement: per-field inheritance of a partial `address` (`offset` is
always the entry's own, `schemas/jmap.schema.json:247`).
- Necessary: no existing map has an address without `offset`.
- `TestSecondaryBufferAddressWithoutOffsetRejected`: a present
`secondaryBufferAddress` without `offset` is rejected.
- Requirement: per-field inheritance of a partial `address`
(`secondaryBufferAddress`).
- Necessary: no existing map has a secondary address without `offset`, and
the secondary resolution is its own code path.
- `TestChannelsEntryWithoutAncestorAddressRejected`: an entry with `channels`
and no address anywhere in its ancestry is rejected.
- Requirement: register or module, and the inheritance chain.
- Necessary: no existing map has a `channels` entry without an address in
its ancestry.
- `TestRepresentationWithoutAncestorAddressRejected`: an entry with a
`representation` and no address anywhere in its ancestry is rejected.
- Requirement: register or module, and the inheritance chain.
- Necessary: no existing map has a register without any address in its
ancestry. The case cannot tell the wording apart, since messages are not
asserted; it pins that the entry fails rather than starting to parse.
- `TestSecondaryBufferAddressInheritsTypeAndChannel`: a double-buffered
register whose primary address is `DMA` on a non-zero channel and whose
`secondaryBufferAddress` gives only `offset`; it parses, with the secondary
on the primary's channel.
- Requirement: a `secondaryBufferAddress` inherits an omitted
`type`/`channel` from the register's resolved address.
- Necessary: every `secondaryBufferAddress` in the existing maps gives
`type` explicitly, and the secondary resolution is its own code path
(`DoubleBufferingInfo::fill`), so the rule needs an assertion there too.
- `TestSecondaryBufferAddressOwnTypeDropsChannelRejected`: in the same
fixture, a `secondaryBufferAddress` giving its own `type` and `offset` but
omitting `channel` does not take the primary's channel over and is rejected
by the same-BAR check.
- Requirement: per-field inheritance of a partial `address`
(`secondaryBufferAddress`, the `type`/`channel` tie).
- Necessary: the case above covers the inherited pair only; this is the only
case with an own secondary `type`.
- `TestAccessInheritanceWithOwnAddress`: an address-bearing parent with
`access: RO` and a child whose own `address` carries only `offset`; the
child is `READ_ONLY`. The parent also carries a `representation`,
`engineeringUnit` and `description`, and the child keeps its own
`representation` with an empty `engineeringUnit` and `description`, so none
of the three is inherited.
- Requirement: inheritance of the register properties (`access`); the
parent's `representation`, `engineeringUnit` and `description` stay its
own.
- Necessary: no fixture combines an own `address` with an omitted `access`
below a parent whose access is not the default.
- `TestTriggeredByInterruptInheritance`: an address-bearing parent with
`triggeredByInterrupt` and two address-less children carrying a
`representation`; the one without `access` is an `INTERRUPT` register with
the parent's interrupt id, the one giving `access: RO` is `READ_ONLY`
without an interrupt id.
- Requirement: inheritance of the register properties
(`triggeredByInterrupt`).
- Necessary: no fixture reaches this rule, since the interrupt registers in
the maps use `channels` and their bit-field children are a reduced struct
that never reaches `fill`. The interrupt id is what the parent has to pass
on, so the case covers the fix.
- Not repeated, already covered: an `address` giving only `offset` with no
address-bearing ancestor defaults to `IO`/`0`
(`tests/selectedByInheritance.jmap`, e.g. `APP.OUTPUT_SELECT`, below the
address-less module `APP`); a secondary on the same BAR is accepted
(`DAQ.CTRL` in `TestGoodMapFileParse`) and a different BAR is rejected
(`TestDoubleBufferDiffBarThrows`). Those fixtures and tests stay untouched.

## Deferred issue

- DI-1 [DONE] The plan cites the schema reference rule at
  schemas/jmap.schema.json:178 twice, but line 178 is the
  access/triggeredByInterrupt exclusivity rule. The channels-inherit-address
  text is at :144 and :203; correct both citations.
- DI-2 [DONE] The test plan's first case is labelled 'Necessary' on the claim
  that no existing map has a partial child address. This is factually wrong:
  tests/simpleJsonFile.jmap already has BSP, an address-bearing parent with
  partial child addresses BSP.VERSION ({offset 4}) and BSP.SOME_INFO ({channel
  0, offset 8}). Re-state the motivation for the case rather than asserting
  non-existence.
- DI-3 [DONE] The test plan's 'Not repeated' bullet mis-attributes coverage:
  MY_INTC is defined in tests/simpleJsonFile.jmap with an explicit channel 4,
  not in tests/selectedByInheritance.jmap, and it does not give only offset.
  Correct the fixture and field attribution.
- DI-4 [DONE] secondaryBufferAddress/offset inconsistency: the requirements
  demand that both the format documentation and the specification state a
  secondaryBufferAddress inherits its omitted fields, but the specification
  keeps SecondAddress::offset at its default 0 while simultaneously rejecting a
  present address without offset. The schema types both primary and secondary
  addresses as #/definitions/address with offset required, so the rejection rule
  and the inheritance claim must be reco...
- DI-5 [DONE] Implementation defect in Address resolution: an address that gives
  its own 'channel' but omits 'type' does not keep the own channel.
  Specification says 'channel is its own value, else the inherited one where the
  type is inherited as well, else 0', and the test plan's own-channel case
  expects the own channel to win. resolveAddress
  (src/JsonMapFileParser.cc:306-320) unconditionally overwrites resolved.channel
  with the inherited channel whenever the type is inherited, because A...
- DI-6 [DONE] Implementation defect in secondaryBufferAddress offset rejection:
  a present secondaryBufferAddress without 'offset' must be rejected with
  ChimeraTK::logic_error like a primary 'address', but DoubleBufferingInfo::fill
  (src/JsonMapFileParser.cc:153) only asserts
  (assert(secondaryBufferAddress.offset.v !=
  std::numeric_limits<size_t>::max())). In a debug build the assert aborts
  (SIGABRT) instead of throwing, so
  TestSecondaryBufferAddressWithoutOffsetRejected fails and aborts the...
- DI-7 [DONE] tests/executables_src/testJsonMapFileParser.cpp
  (TestChannelsEntryInheritsAddressBecomesRegister): the second assertion block
  fetches '/APP/CHANS_PARENT/CHANS_CHILD/B', which is the channel slice named B,
  but its comment claims 'A bit-field child of a channel resolves to the same
  address as well'. Channel B is declared as a plain channel ({bytesPerElement
  4, offset 0, representation {type fixedPoint, width 4}}) with no 'children',
  so no bit-field child (a 'children' entry belo...
- DI-8 [DONE] The user requests replacing the 'unset' sentinels with
  std::optional: Address::type/channel/offset and
  DoubleBufferingInfo::SecondAddress::type/channel/offset currently use
  addressTypeNotSet / std::numeric_limits<size_t>::max() to mark an omitted
  (inherited) field (src/JsonMapFileParser.cc:134-136, :179-181). They should
  become std::optional, with an omitted JSON field deserialising to std::nullopt
  and resolveAddress (:304-331) and DoubleBufferingInfo::fill (:146-171)
  testin...
- DI-9 [FOLDED-IN] Mechanism (Specifications, Recursion): the implemented parser
  keeps the inherited state in `AncestorRegister` and passes it as a second
  argument next to the resolved address (`src/JsonMapFileParser.cc:120-128`,
  `:493`, `:511`, `:518`, `:521`, `:617`). It becomes a `derived` member of
  `JsonAddressSpaceEntry` resolved by `derive()`; `addInfos` takes the parent
  entry and hands `this` down; `fill` (`:332-440`) drops `resolved`, `ancestor`
  and both `ownsAddress` branches; `applyRegisterSelectedBy` (`:442-452`) folds
  into `fill`; the child loop (`:616`) and the top-level loop (`:724`) iterate
  non-const. Reasons and rejected alternatives are in 'Alternatives considered'.
- DI-10 [FOLDED-IN] `access` inheritance: implemented only for an entry that
  omits its whole address (`:346-353`); it must apply whenever the entry gives
  no `access`, with an own `address` included. Covered by the new test plan case
  for `access` inheritance.
- DI-11 [FOLDED-IN] `triggeredByInterrupt` inheritance: not inherited today, so
  an address-less child of an interrupt register becomes `Access::INTERRUPT`
  with an empty `interruptId` (`:342-362`). It must inherit the parent's sources
  together with that access unless the entry gives its own `access`. Covered by
  the new test plan case for interrupt inheritance; no fixture reaches the rule.
- DI-12 [FOLDED-IN] Sample geometry: the behaviour stays as implemented
  (`:371-383`, inherited iff the whole address is omitted and the entry gives no
  `channels`, an own `bytesPerElement` overriding the sample width). Only where
  the rule is expressed changes, `derive()` instead of `fill`. No fixture or
  test change.
- DI-13 [FOLDED-IN] `doc/jmapFormat.dox:242-261`: the example claims an
  identical outcome to the one above it, but its `STATUS` child omits the
  address and the implemented parser inherits `numberOfElements: 2` of `APP` for
  it, so the claim is wrong. The example or the claim must be corrected.
- DI-14 [FOLDED-IN] Remove the temporary section once DI-9 to DI-13 are done.
- DI-15 [FOLDED-IN] The documentation and schema plan does not state the
  register-property inheritance, contradicting the synopsis ("register-property
  inheritance that doc/jmapFormat.dox and schemas/jmap.schema.json describe ...
  Both documents must state that rule precisely") and the Requirements aspect
  "inheritance of the register properties". The Requirements aspect
  "documentation and schema" enumerates only the
  address/type-channel/module/secondary rules, and the Specification subsections
  `Format documentation` and `JSON schema` likewise list no statement of
  `access`, `triggeredByInterrupt` or `numberOfElements`/`bytesPerElement`
  inheritance. `access` inheritance is a behaviour change (DI-10), yet
  `doc/jmapFormat.dox` (jmap_access, jmap_address_inheritance) and the schema
  descriptions of `access` and `triggeredByInterrupt` stay silent, and the
  `children` schema description (`schemas/jmap.schema.json:153`) still says
  "inherits the parent's address" instead of the nearest ancestor's.
- DI-16 [FOLDED-IN] Specification, `Derived` field bullet: "The pitch is
  `sampleWidthBytes * 8` for a 1D entry" contradicts the derive rule further
  down, where an address-less entry without `channels` inherits the parent's
  `elementPitchBits` while an own `bytesPerElement` overrides only the sample
  width (DI-12). The sentence must be limited to an entry that gives its own
  `address`, or it invites computing `elementPitchBits = sampleWidthBytes * 8`
  for inherited entries and thereby changing the sample pitch.
- DI-17 [FOLDED-IN] Requirement "An entry with a `representation` or with
  `channels` and no address anywhere in the ancestry is rejected" contradicts
  the Specifications: a void-typed entry has a `representation` and requires no
  address (existing fixtures BSP.VOID_INTERRUPT_0/3_0_1), and `fill` explicitly
  exempts it. The requirement must say a non-void `representation`.
- DI-18 [FOLDED-IN] The document no longer uses the fixed structure: all list
  items are at the same level. The per-member sub-bullets of the Specifications,
  the per-test-case `Requirement`/`Necessary` bullets of the Test plan and the
  reasons of the rejected `NumericAddressedRegisterInfo` alternative are
  top-level peers, so e.g. "`bar` folds ...", "A reference to it would point at
  a local ..." and "The entry that declared the effective selector ..." read as
  standalone alternatives instead of reasons under "Carrying the filled
  `NumericAddressedRegisterInfo` of the parent instead of `Derived`: rejected."
  The playbook requires two-space indentation per nesting level.
- DI-19 [FOLDED-IN] Test plan, `TestAccessInheritanceWithOwnAddress`: the bullet
  claims the case asserts that the parent's `representation`, `engineeringUnit`
  and `description` do not leak, but the stated assertion is only that the child
  keeps its own `representation`, which is its own value and proves nothing
  about inheritance (and says nothing about `engineeringUnit`/`description`).
  Either assert the child's empty `engineeringUnit` and `description` while the
  parent sets them, or drop the claim.
