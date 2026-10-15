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
  (`schemas/jmap.schema.json:178`): `channels` alone makes it one, and its
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
  instead. `schemas/jmap.schema.json:178`, which already lets a 2D register
  inherit its address, is correct and stays as it is; the parser was the side
  that deviated.
- Both additionally state that a `secondaryBufferAddress` inherits its omitted
  fields from the register's address, with the same `type`/`channel` tie.

## Specifications

Affected components: `src/JsonMapFileParser.cc`, `schemas/jmap.schema.json`,
`doc/jmapFormat.dox` (wording only).

Field presence:

- `Address::type` and `Address::channel` get unset sentinels as defaults, so a
  field given in the map is told apart from an omitted one. Currently both are
  pre-set to `IO`/`0` and the `WITH_DEFAULT` deserialisation keeps them, which is
  what loses the inheritance.
- `Address::offset` keeps its sentinel, and a present `address` without `offset`
  is now rejected instead of silently using the sentinel value; the schema
  already requires `offset` (`schemas/jmap.schema.json:247`).
- `JsonAddressSpaceEntry::address` becomes `std::optional<Address>`. A present
  address with unset fields means "inherit" per the resolution rule below; an
  absent address means "inherit the whole ancestor address". This replaces the
  current `type == addressTypeNotSet` absence test (`:167`, `:299`, `:339`,
  `:444`, `:554`).
- `DoubleBufferingInfo::SecondAddress` uses the same sentinels for
  `type`/`channel` instead of the `DMA`/`0` defaults; its `offset` keeps the `0`
  default.

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

- `addInfos` passes the nearest ancestor's resolved address down the recursion,
  replacing the `addressSetByParent` flag (`:276`, `:285`, `:319`, `:432`,
  `:448`, `:546`, `:554`).
- An entry with a present `address` resolves `type` from its own value, else
  from the inherited address, else from the `IO` default. `channel` is its own
  value, else the inherited one where the `type` is inherited as well, else `0`.
  `offset` is always its own.
- The resolved address becomes the inherited address of the entry's children, so
  inheritance works through modules and from a 2D register down to its channels
  and their bit fields.
- Whole-address omission no longer looks the parent up in the catalogue
  (`catalogue.getBackendRegister(parentName)`, `:544`); the address comes from
  the ancestor state carried down the recursion. This is the same top-down
  resolution as the per-field merge.
- The recursion carries the ancestor's register info as well, because an entry
  with an omitted address takes the ancestor's `elementPitchBits` for the
  `bytesPerElement` fallback (`:326`) and keeps its `registerAccess` (`:285`).

Double buffering:

- `DoubleBufferingInfo::fill` resolves the secondary `type`/`channel` against the
  register's resolved address by the same rule, so an omitted `type` brings the
  register's channel along, before computing the secondary BAR.
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
- Making every entry below an address-bearing ancestor a register, its omitted
  `representation` defaulting to 32-bit (`doc/jmapFormat.dox:104`): rejected, it
  would add a register for every grouping module, against
  `doc/jmapFormat.dox:196` ("A module is not a register and carries no data of
  its own").

## Test plan

All test cases go into `tests/executables_src/testJsonMapFileParser.cpp`. The
positive cases extend `tests/simpleJsonFile.jmap`; the rejection cases erase an
`address` from a copy of it, like the existing selectedBy and map-format-version
helpers, so no fault-only fixtures are added. The shared fixtures are used by the
backend test binaries as well, which no case here affects. A rejection case
expects a `ChimeraTK::logic_error` and nothing more; no case asserts an error
message.

- The documented inheritance example (`doc/jmapFormat.dox:242-261`): an
  address-bearing parent on `IO` channel 2 with one child carrying only a
  `representation` and one child whose `address` gives only `offset`; both
  children land on the parent's channel (the second currently on channel 0).
  - Requirement: per-field inheritance of a partial `address`.
  - Necessary: no existing map has an address-bearing ancestor with a partial
    child `address`; the `APP.STATUS` children of `tests/simpleJsonFile.jmap`
    inherit a whole address, which is a different code path that works today.
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
  below an address-bearing entry: yields a register on that address, and its
  channels and their bit-field `children` resolve to the same address (currently
  dropped silently).
  - Requirement: register or module, and the inheritance chain (`channels` alone
    makes a register).
  - Necessary: every `channels` entry in the existing maps carries its own
    `address`.
- An address-bearing entry with a module child holding an address-less child
  that carries a `representation`: the address reaches the child through the
  module (currently `Register ... does not exist`), and the module itself yields
  no register.
  - Requirement: register or module, and the inheritance chain.
  - Necessary: the existing maps have modules above a register-bearing entry
    (`APP`, `MY_INTC`) and a module without any address (`MY_INTC.SUB0`), but no
    module between an address-bearing entry and an address-less descendant. One
    module level suffices, the pass-through is the recursion itself.
- A present `address` without `offset`: rejected.
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
  BARs" failure). In the same fixture, a `secondaryBufferAddress` giving only its
  own `type` does not take the primary's channel over and is rejected by the
  same-BAR check.
  - Requirement: `secondaryBufferAddress` inherits an omitted `type`/`channel`
    from the register's resolved address, with the `type`/`channel` tie.
  - Necessary: every `secondaryBufferAddress` in the existing maps gives `type`
    explicitly, and the secondary resolution is its own code path
    (`DoubleBufferingInfo::fill`), so the tie needs an assertion there too.
- Not repeated, already covered today: an `address` giving only `offset` with no
  address-bearing ancestor defaults to `IO`/`0`
  (`tests/selectedByInheritance.jmap`, the `MY_INTC` entries); a secondary on
  the same BAR is accepted (`DAQ.CTRL` in `TestGoodMapFileParse`) and a
  different BAR is rejected (`TestDoubleBufferDiffBarThrows`). Those fixtures and
  tests stay untouched.
