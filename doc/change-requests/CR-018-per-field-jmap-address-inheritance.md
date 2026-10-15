# CR-018: per-field inheritance of JMAP addresses

Synopsis: The parser must implement the per-field address and register-property
inheritance that `doc/jmapFormat.dox` and `schemas/jmap.schema.json` describe.
Both documents must state that rule precisely.

Status: IN PROGRESS

## Requirements

Aspect: per-field inheritance of a partial `address`.

- A present `address` gives its own `offset`, as the schema's `address`
  definition requires; only `type` and `channel` may be omitted.
- An omitted `type` inherits the `type` of the nearest ancestor entry that has an
  address, and with it that ancestor's `channel`.
- An own `channel` wins over the inherited one.
- An own `type` drops the inherited `channel`, so an omitted `channel` is then
  `0`. A channel means something different per `type`, so it is not carried
  across a type change.
- Where no ancestor has an address, an omitted `type` and `channel` default to
  `IO` and `0`.
- A wholly omitted `address` also inherits the ancestor's `offset`.
- A `secondaryBufferAddress` inherits an omitted `type` and `channel` from the
  primary address resolved for the register.
- The `offset` of a `secondaryBufferAddress` is its own and required, so a
  `doubleBuffering` object whose `secondaryBufferAddress` omits it, or which has
  no `secondaryBufferAddress` at all, is rejected.
- A secondary `type`/`channel` that resolves to a BAR other than the primary one
  is rejected.

Aspect: register or module, and the inheritance chain.

- An entry with an address in its ancestry, own or inherited, is a register.
- An entry that gives a `representation` or `channels` is a register as well,
  which is how a void-typed entry is a register without an own `address`.
- An entry with neither is a module.
- A module groups nested entries and carries no data of its own. It may give
  `children`, `selectedBy`, `engineeringUnit` and `description`.
- A module that gives `access`, `triggeredByInterrupt`, `numberOfElements`,
  `bytesPerElement`, `pitch` or `doubleBuffering` is rejected.
- A register without a `representation` and without `channels` is a fixed-point
  word of its effective sample width at its resolved address. A grouping entry
  below an address is such a register, so it parses next to its children.
- An entry with `channels` is a register on the address that resolves for it,
  with or without a non-void `representation` and with or without an own
  `address`.
- An entry with a non-void `representation` or with `channels` is rejected where
  it has no address on the way up to the nearest void entry or the top.
- A void-typed entry requires no own `address`.
- A void-typed entry carries no data word. A `doubleBuffering`, `channels`, an
  own `numberOfElements`, `bytesPerElement` or `pitch` on it is rejected, and an
  own `address` as well.
- A void-typed entry takes no address from its ancestors either, so the address
  chain below it starts afresh and its children are in the position of top-level
  entries. This barrier concerns the address only: the `access` and
  `triggeredByInterrupt` that a void-typed entry gives reach its descendants like
  any other register's.
- A child inherits from the nearest ancestor that has an address, and the search
  stops at a void entry.
- Every entry in between inherits that same address too, since an entry below an
  address is a register.
- The channel entries below `channels` never carry an address of their own and
  take the resolved address of their direct 2D parent, which is always a
  register. The nested bit-field `children` of a channel do the same
  (`schemas/jmap.schema.json`, the `channels` and `channelChild` definitions).

Aspect: inheritance of the register properties.

- `access` is inherited from the nearest ancestor register wherever the entry
  gives no `access` and no `triggeredByInterrupt` of its own, with an `address`
  or without.
- `triggeredByInterrupt` is inherited together with the `access` it implies,
  unless the entry gives an `access` of its own. Giving `access` is the way to
  keep a child a plain register.
- `access` and `triggeredByInterrupt` remain mutually exclusive on a single entry
  (`schemas/jmap.schema.json`, the `access` and `triggeredByInterrupt` definitions).
- A void-typed entry inherits neither `access` nor `triggeredByInterrupt`; it
  takes its own `triggeredByInterrupt`, which it must give.
- The geometry is inherited only where the entry omits its whole `address` and
  gives no `channels`, since it describes the sample the entry is cut from.
- An own `numberOfElements` or `bytesPerElement` overrides the inherited value.
- An own `bytesPerElement` changes the sample width only; the element pitch the
  entry is strided by stays the inherited one.
- Below a 2D ancestor the sample width is that ancestor's `pitch`, since a 2D
  register describes its sample through `pitch` and not through
  `bytesPerElement`.
- The bit-field `children` of a channel take the channel's `bytesPerElement` as
  their word context (`doc/jmapFormat.dox`, section `jmap_bitfields`), so they
  must not give one of their own.
- A geometry value of `0` is rejected: `numberOfElements`, `bytesPerElement` and
  `pitch` are optional members, so an absent key states "not set" and `0` does
  not.
- Every entry of `channels` gives its `offset`; one that omits it is rejected,
  since `0` is a valid offset and cannot stand for "not set".
- A `channels` entry must give `pitch`.
- `pitch` is invalid without `channels`.
- `selectedBy` is inherited from the nearest ancestor entry, module or register.
- A register-level `selectedBy` next to `channels` applies to the register's
  channels.
- `representation`, `engineeringUnit` and `description` are never inherited.

Aspect: documentation and schema.

- `doc/jmapFormat.dox` and `schemas/jmap.schema.json` must state every rule above
  that they still lack.

Aspect: compatibility of existing maps.

- The map format version stays `1.0`: the stricter grammar and the changed
  inheritance of `access` described above apply to every file that declares it.
- A map that parses today stops parsing where it gives an `address` without
  `offset`, a `doubleBuffering` without a `secondaryBufferAddress` or with an
  offset-less one, a `representation`, `channels` or a data member on a module, a
  `doubleBuffering`, `channels`, an own `address` or an own geometry on a
  void-typed entry, a `pitch` without `channels`, a geometry value of `0`, or a
  channel without `offset`.
- A register that gives its own `address` and no `access` below an ancestor whose
  access is not `READ_WRITE` keeps parsing and resolves differently: it takes the
  inherited access and interrupt instead of `READ_WRITE`.

Aspect: the tools that write JMAP files.

- `chimeratk-device-to-jmap` and `chimeratk-map-to-jmap` emit maps that satisfy
  the rules above: every `address` carries its `offset`, no geometry value is
  `0`, `type` is always explicit, and `access` is explicit except on an interrupt
  register, which carries `triggeredByInterrupt` instead.
- A register whose size the source does not determine is left out by
  `chimeratk-device-to-jmap` and written without `bytesPerElement` by
  `chimeratk-map-to-jmap`, so a converted map keeps the meaning it has today; in
  both cases the tool reports the register it could not size on stderr.

## Specifications

### JsonAddressSpaceEntry (modified) in src/JsonMapFileParser.cc (modified)

- Data members become private. All inheritance is resolved by `const` getters that
  consult the parent entry; only the linking pass is non-`const`. Each rule is
  stated in one getter only, and no resolved value is threaded down the parsing
  recursion.
  - `NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT` stays inside the class body, so
    the private members deserialise without accessors or a friend declaration.
  - Member names stay the JSON keys. The getters carry an `effective` prefix, since
    a data member and a member function cannot share a name.
  - `numberOfElements`, `bytesPerElement` and `pitch` become `std::optional<size_t>`,
    so an own value is told from an omitted one and a module can reject them.
  - `access` becomes `std::optional<Access>`, and `Access::accessNotSet` is
    deleted. Its value collided with
    `NumericAddressedRegisterInfo::Access::INTERRUPT`; with the sentinel gone, the
    getter expresses the interrupt case with the catalogue's enum.
  - `parentLink` and `fullPath` are not in the macro's member list, so they are
    neither read from nor written to JSON.
- `size_t Address::bar() const` (new): `channel + (type == DMA ? 13 : 0)`, the one
  place for the BAR formula, which `Address::fill` and the secondary same-BAR
  check both use. It reads `type`/`channel` as values, so it
  asserts that `resolve` ran, like `Address::fill`.
- `struct ParentLink` (new): `const JsonAddressSpaceEntry* parent`, null for a
  top-level entry, and `bool linked`.
  - The pointer refers into the tree of entries. A parent owns its sub-entries
    through `std::map` nodes, so it outlives its child, and no pointer leaves the
    tree: `addressSpace` is a local of the parse function and only copies of
    `NumericAddressedRegisterInfo` leave it.
  - The entry's move operations assert `!other.linked` through `ParentLink`'s move
    operations, so a move after linking trips the assertion instead of leaving a
    second object that claims a parent inside the tree. Parsing moves an entry into
    its map node before linking, and `std::map` moves nodes but never elements.
  - The assertions are `assert`s rather than exceptions because neither state is
    reachable from user input: `parse` runs the whole linking pass before it
    creates the first register, and no entry leaves the parser, so a getter
    reached before the pass is a bug inside the parser. `parse` is the only path
    from a map file to these entries.
  - The entry stays copyable, because `NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT`
    expands to `j.value(key, default.x)`, which copies the default `children` map
    and with it every entry, so deleted copy operations would not compile. All
    copies happen while parsing, before the linking pass, so no linked entry is
    copied; the copy operations assert that.
- `void linkTo(const std::string& name, const JsonAddressSpaceEntry* parent)` (new): the
  single linking pass, run once per top-level entry after parsing.
  - Rejects an empty `name` with `logic_error`, taking over the check of the
    current `addInfos`.
  - Sets `parentLink.parent`, `parentLink.linked` and `fullPath`, then recurses
    into the sub-entries.
  - `fullPath` becomes `parent->path() / name` with the dotted alt separator, and
    `/` + `name` at the top level, as today.
  - Asserts that the entry is not linked yet, so a repeated pass cannot rebuild the
    links silently.
- `const RegisterPath& path() const` (new): `fullPath`, the fully qualified path of
  the entry. The getters assert `parentLink.linked`, so a resolution before the
  pass asserts instead of reading an empty path.
  - It replaces the `name` and `parentName` arguments of `fill` and `addInfos`, and
    is what the error messages and `info.pathName` use.
  - It needs no second lookup to name the entry that declared the effective
    selector.
- `bool isRegister() const` (new): an entry for which an address resolves, own
  or inherited, or which gives a `representation` or `channels`; an entry with
  none of these is a module. A void-typed entry is thus a register without an
  address, which is why this test reads the members and not `effectiveAddress()`.
- `std::optional<Address> effectiveAddress() const` (new): the resolved address.
  - An own `address` resolves with `Address::resolveTypeAndChannel` to the nearest
    ancestor's `type`/`channel`; its `offset` is its own and mandatory.
  - Without an own `address` the entry returns the parent's, and `std::nullopt`
    where no ancestor has an address.
  - A void-typed entry returns `std::nullopt` whatever its ancestors give, so it
    is the one register without a resolved address; it consequently neither
    writes an address into the register info nor passes one on, and the search of
    a child stops at it.
  - The rejection of a present `address` without `offset` happens in `addInfos`
    before any child resolves an address, so no assertion is needed here.
- `NumericAddressedRegisterInfo::Access effectiveAccess() const` (new): the own
  `access`; else `INTERRUPT` where the entry gives its own `triggeredByInterrupt`;
  else the parent's, and `READ_WRITE` at the top. The catalogue's enum is the
  return type because it has an `INTERRUPT` value; the own `access` member stays
  the parser's `Access`, converted here once.
  - A module gives no `access` of its own, so it passes the ancestor's value
    through.
  - A VOID-typed entry inherits neither: it takes its own `triggeredByInterrupt`,
    which gives it `INTERRUPT`. Its descendants inherit that value and its
    interrupt id like any other register's.
- `std::vector<size_t> effectiveInterruptId() const` (new): the own
  `triggeredByInterrupt`; else the parent's where `effectiveAccess()` is
  `INTERRUPT`; empty where the entry gives an `access` of its own, which
  suppresses the inheritance. An own `access` next to an own
  `triggeredByInterrupt` stays rejected, so the `access`/
  `triggeredByInterrupt` tie lives in `effectiveAccess()` alone and this getter
  derives from it.
- `size_t effectiveNumberOfElements() const`, `size_t effectiveSampleWidthBytes()
  const` and `size_t effectiveElementPitchBits() const` (new): the geometry is
  inherited per JSON field, and only where the entry omits its whole `address`
  and gives no `channels`; everywhere else each getter returns the entry's own
  value, and `1` and `4` where it gives none.
  - Where the entry omits its whole `address` and gives no `channels`, it inherits
    the element pitch from the nearest ancestor register as well, and an own
    `bytesPerElement` changes only the sample width, not the stride, as today.
    Below a 2D ancestor the sample width is that ancestor's `pitch`.
  - At the top there is no ancestor, so the defaults are the own values or `1` and
    `4`, and the pitch `sampleWidthBytes * 8`.
  - Otherwise the pitch is derived from the entry's own geometry: an entry with
    `channels` uses its own `pitch * 8` after `addInfos` rejected a `channels`
    entry without `pitch`, and an entry with an own `address` and no `channels`
    uses `effectiveSampleWidthBytes() * 8`.
  - Bit-field `children` of a channel take the channel's `bytesPerElement` as
    their word context (`doc/jmapFormat.dox`, section `jmap_bitfields`), so an own
    one changes the word the child is cut from. The parser ignores the key as it
    ignores every key a definition does not list, and the schema forbids it;
    `addInfos` needs no marker member for it.
- `std::optional<SelectedBy> effectiveSelectedBy() const` and `RegisterPath
  selectedBySource() const` (new): the own value, else the parent's, for a module
  as well; the source is `path()` where the entry declares it, else the parent's,
  and empty at the top, as the `addInfos` argument is today.
- `Representation`, `engineeringUnit`, `description`, `doubleBuffering` and
  `channels` are never inherited. The class's own members read them directly, so
  they need no getter. `pitch` is an own member too.
- `void fill(NumericAddressedRegisterInfo& info) const` (modified): writes
  `info.pathName` from `path()` and the non-inheritable members, and takes every
  other value from the getters. It holds no inheritance logic and needs neither a
  path argument nor the catalogue.
  - It applies the register-level selector through `applyRegisterSelectedBy`,
    whose signature stays as it is, taking `effectiveSelectedBy()` and
    `selectedBySource()` from the getters. It applies the selector where the entry
    gives no `channels` and is no VOID, and passes `effectiveSelectedBy()` into the
    channel fill, which `Channel::fill` and the slices use as today. A
    register-level selector next to `channels` is not applied to the register
    itself, as today.
  - It rejects an own `access` next to an own `triggeredByInterrupt`, the check
    that sits in the entry branch today.
  - It writes an own `representation` only where the entry gives no `channels`;
    `Representation::fill` would otherwise append a channel to a 2D register.
  - A register that gives no `representation` and no `channels` gets a fixed-point
    `Representation` whose `width` is the effective sample width in bits and whose
    `bytesPerElement` is the effective value, so `bytesPerElement: 8` yields a
    64-bit word. A grouping entry below an address thus yields a raw-word register
    next to its children.
  - A `representation` without `width` keeps the default of `32`, which the
    member initialiser provides; an omitted entry-level `representation` leaves
    the member at
    `representationNotSet`, which `isRegister()` and the missing-address check
    test.
  - It clears `info.channels` before filling, since the default constructor of
    `NumericAddressedRegisterInfo` already creates a channel; `addInfos` no
    longer clears it.
  - A VOID-typed entry writes `elementPitchBits` 0, `nElements` 0 and the `nodata`
    descriptor and leaves `bar`/`address` empty. It keeps requiring an own
    `triggeredByInterrupt`, and `addInfos` rejects a `doubleBuffering`, `channels`,
    an own `address` or an own geometry on it.
- `void addInfos(NumericAddressedRegisterCatalogue& catalogue) const` (modified):
  rejects an own `address` without `offset`, creates a register for `isRegister()`
  entries through `fill`, and recurses into the sub-entries.
  - The parent entry argument is gone, so no state is passed down and the pass is
    `const`.
  - It rejects every non-void entry for which `isRegister()` holds and
    `effectiveAddress()` is empty, with `Address not set but representation or
    channels given`. This covers a `representation` and `channels`, so the rule
    lives in one place and `fill` never sees a non-void register without an
    address; a void-typed entry has none by definition and passes.
  - It runs its own rejections before it calls `fill`, so `fill` never reads a
    value they guard. `fill` rejects only what concerns the entry's own data
    layout: the `access`/`triggeredByInterrupt` pair and the VOID restrictions.
  - It no longer copies the parent's register out of the catalogue, which
    needed the direct parent to be a register: the getters reach the nearest
    ancestor that has an address.
  - It rejects a channel without `offset`, a `channels` entry without `pitch`, an
    own `pitch` on a register without `channels`, and a `numberOfElements`,
    `bytesPerElement` or `pitch` of `0` on an entry or a channel.
  - Its checks run in a fixed order, the VOID ones first, so a VOID entry with an
    own `pitch` reports the VOID rule rather than the generic `pitch` rule.
  - It rejects a VOID entry with a `doubleBuffering`, `channels`, an own `address`
    or an own `numberOfElements`, `bytesPerElement` or `pitch`, since such an entry
    has no data word.
  - It calls `info.computeDataDescriptor()` after `fill`, as today,
    so the descriptor matches the filled channels.
  - It enters every register through `addSliceWithBufferViews`, which
    adds the register itself and creates the `BUF0`/`BUF1` views only where
    `info.doubleBuffer` is set, so no second `catalogue.addRegister` is needed.
  - The view creation follows `fill`, which sets `info.doubleBuffer` for every
    entry that has the member.
  - The `BUF1` view takes its address from `info.doubleBuffer->address`,
    which `DoubleBufferingInfo::fill` resolved, rather than from a secondary
    `offset` that may be absent.
  - A module that gives `access`, `triggeredByInterrupt`, `numberOfElements`,
    `bytesPerElement`, `pitch` or `doubleBuffering` is rejected with
    `logic_error`.
- `Channel::offset` becomes `std::optional<size_t>`, so a channel that
  omits it is rejected instead of reading an indeterminate default; a channel at
  offset `0` stays valid, which is why the member is optional rather than
  rejected on a value. `addInfos` rejects it before `Channel::fill` dereferences
  it, before `channelsInOffsetOrder` sorts on it and before the slice
  address uses it; the schema's `channel` definition already requires it.
- Channel and bit-field slices are created for every register with `channels`,
  at its resolved address, so an address-less `channels` entry gets them too.
  - A channel slice takes the register's resolved address plus the channel's
    `offset`, its `bytesPerElement` and the register's geometry; the bit-field
    `children` of a channel work the same way off the channel's word.

### JsonAddressSpaceEntry::Address (modified) in src/JsonMapFileParser.cc (modified)

- `std::optional<Address> address` in `JsonAddressSpaceEntry`, so an omitted
  `address` is a `std::nullopt` and an `address: {}` is a present value whose
  fields are all omitted.
  - `isRegister()` tests the presence, `effectiveAddress()` resolves a present
    one, and `addInfos` rejects a present one without `offset` before anything
    resolves.
- `std::optional<AddressType> type`, `std::optional<size_t> channel` and
  `std::optional<HexValue> offset`. `AddressType::addressTypeNotSet` goes
  with them, since no code needs the sentinel any more.
  - An omitted field deserialises to `std::nullopt`, so a field given in the map
    is told apart from an omitted one. `WITH_DEFAULT` keeps that absence.
- `static std::pair<AddressType, size_t> resolveTypeAndChannel(const
  std::optional<AddressType>& ownType, const std::optional<size_t>& ownChannel,
  const std::optional<Address>& inherited)` (new): the single copy of the
  `type`/`channel` tie, used by the primary and the secondary address.
  - An own `type` wins; an omitted `type` takes the inherited `type` together
    with its `channel`; with no inherited address it becomes `IO` and the
    `channel` `0`.
  - An own `channel` always wins.
- `static Address resolve(const Address& own, const std::optional<Address>&
  inherited)` (new): resolves an own address to a fully populated `Address`, its
  `offset` taken from the own one. `effectiveAddress()` returns the result, so
  `Address::fill` writes `info.address` and `info.bar` from it and
  reads `*offset`; its `addressTypeNotSet` assertion becomes an assertion that
  `resolve` ran. A second resolution site is unnecessary.

### JsonAddressSpaceEntry::DoubleBufferingInfo (modified) in src/JsonMapFileParser.cc (modified)

- `std::optional<Address> secondaryBufferAddress` (modified): `struct
  SecondAddress` disappears, since `Address` has the same optional fields. The
  `offset` is optional only to detect that it was omitted, since it is never
  inherited: `WITH_DEFAULT` would otherwise default it to `0`. The whole member is
  optional, so an omitted `secondaryBufferAddress` is told from a present one
  without `offset`.
- `void fill(NumericAddressedRegisterInfo& info, const Address& resolvedPrimary)
  const` (modified): resolves the secondary `type`/`channel` with
  `Address::resolve` against the register's resolved address, before computing the
  secondary BAR.
  - The secondary `offset` is never inherited: a `secondaryBufferAddress`
    without `offset` is rejected with `logic_error`, an omitted
    `secondaryBufferAddress` included. `fill` checks both before it calls
    `Address::resolve`, so the resolved address never dereferences an absent
    `offset`.
  - The same-BAR check takes the primary BAR from `resolvedPrimary.bar()`, the
    address the caller resolved, so its outcome does not depend on what `fill`
    has written into `info` already; the message reports that BAR.

### Format documentation (modified) in doc/jmapFormat.dox (modified)

- `jmap_access`: a register that gives no `access` inherits the access of the
  nearest ancestor register, together with its `triggeredByInterrupt` unless the
  entry gives an `access` of its own; a void-typed entry inherits neither, while
  its own interrupt reaches its descendants like any other register's. This also
  applies to a register with an own `address`, which so far took `READ_WRITE`
  where the map gave no `access`.
- `doubleBuffering` (`doc/jmapFormat.dox` and `DoubleBufferingInfo`): an omitted
  secondary `type` no longer defaults to `DMA`; it inherits from the primary
  address resolved for the register. `secondaryBufferAddress` is mandatory, its
  `offset` always the object's own, and the two buffers must lie on the same BAR.
- `jmap_address_inheritance`: `numberOfElements` and `bytesPerElement` are
  inherited only where the whole `address` is omitted and the entry gives no
  `channels`, an own value of either overriding the inherited one.
- The register section points to the nearest ancestor instead of a parent module.
- `jmap_representation`: a register that omits its whole `representation` and
  gives no `channels` gets a fixed-point word of its effective sample width, not a
  32-bit word.
- `jmap_elements`: `numberOfElements` and `bytesPerElement` are inherited from the
  nearest ancestor register where the entry omits its whole `address` and gives no
  `channels`, own values overriding them, and default to `1`/`4` otherwise. A
  value of `0` is invalid, and below a 2D ancestor the sample width is that
  ancestor's `pitch`.
- `jmap_modules`: an entry without an address on the way up to the nearest void
  entry or the top that gives neither `representation` nor `channels` is a module.
  A module may give `children`, `selectedBy`, `engineeringUnit` and `description`,
  and `access`, `triggeredByInterrupt`, `numberOfElements`, `bytesPerElement`,
  `pitch` and `doubleBuffering` on it are rejected. An entry with `channels` is a
  register on the address that resolves for it, and one with `representation` or
  `channels` but no such address is rejected.
- The `type`, `channel` and `offset` bullets of the `address` object: a present
  `address` always gives `offset`; only `type` and `channel` may be omitted. Both
  inherit from the nearest ancestor, and a `type` of its own drops the inherited
  `channel`, so an omitted `channel` is `0` then. Only where no ancestor has an
  address do they default to `IO`/`0`.
- `jmap_bitfields`: the bit-field `children` of a channel take the channel's
  `bytesPerElement` and must not give one of their own.
- The void-typed entry forbids an own `address` and takes none from its ancestry
  either, so the address chain below it starts afresh; a `doubleBuffering`,
  `channels` or an own geometry on it is rejected. The interrupt example stays as
  it is, its top-level VOID entries having no address.
- The inheritance example states that its `STATUS` child inherits
  `numberOfElements: 2` of `APP`, instead of claiming an identical outcome to the
  example above it.

### JSON schema (modified) in schemas/jmap.schema.json (modified)

- `addressSpaceEntry`: an entry with an address in its ancestry is a
  register; one without is a module that may give `children`, `selectedBy`,
  `engineeringUnit` and `description`, unless it gives `representation` or
  `channels`, which is rejected, or one of the data members, which are rejected
  too. All three rejections are description only, since a per-entry schema cannot
  see whether an address or a void entry sits above the entry.
- `numberOfElements` and the `bytesPerElement` definition: inherit from the
  nearest ancestor register where the entry omits its whole `address` and gives
  no `channels`, and default to `1`/`4` otherwise. Both get `minimum: 1`, which
  covers the register member and the channel's, since `0` is no second spelling of
  "not set"; `pitch` already has it.
- `channels`, `children`, the 2D-register block and `address`: say nearest
  ancestor instead of parent, and `address` states that only `type`/`channel` are
  inherited, `offset` being the entry's own.
- The `type` field and the `channel` field: inherit from the nearest ancestor, a
  `type` of its own dropping the inherited `channel`, and default to `IO`/`0` only
  where no ancestor has an address.
- `width` and the `representation` description: an omitted `representation`
  yields a fixed-point word of the effective sample width, while an explicit one
  without `width` keeps the default of `32`.
- `access` and `triggeredByInterrupt`: state the inheritance.
- The channel-child definition states that its `bytesPerElement` must not be
  given; its `additionalProperties` keeps rejecting unknown keys.
- The void-typed entry: the description says that it forbids an
  own `address` and inherits none, so the address chain below it starts afresh,
  the latter being no constraint a per-entry schema can express;
  `doubleBuffering`, `channels` and the geometry members are rejected as new
  constraints in the `allOf` block.
- `doubleBuffering` gets a `required` list naming `secondaryBufferAddress`,
  which the definition does not have today, so the schema matches the rejection.
  `secondaryBufferAddress` gets its own definition instead of the `$ref` to
  `#/definitions/address`, since the shared description says "nearest
  ancestor" while the secondary inherits from the register's resolved address.

### JsonMapFileParser::parse (modified) in src/JsonMapFileParser.cc (modified)

- Rejects exactly the maps that the compatibility aspect of the requirements
  lists, without a format version bump: its version check stays untouched, while
  `addInfos`, `fill` and `linkTo` own the individual rejections specified above.
- Runs `linkTo` for every top-level entry after parsing and before `addInfos`,
  which is also where `addInfos` is called with its new signature. The loop binds
  its entries by `auto&` instead of `const auto&`, since `linkTo` writes into the
  entry.

### chimeratk-device-to-jmap (modified) in tools/src/chimeratk-device-to-jmap.cc (modified)

- Skips a register whose `byteSizePerElement` is `0` (for an `undefined`
  descriptor), which it would otherwise emit with `bytesPerElement: 0`, and
  reports the skipped register on stderr.
- No change is needed for its VOID registers, since a void-typed entry needs no
  address in its ancestry.

### chimeratk-map-to-jmap (modified) in tools/chimeratk-map-to-jmap (modified)

- Writes `type` and `access` explicitly, since both are inherited now:
  `_make_address` omits `"type"` for `IO` and the serialiser omits a default `RW`
  `access`, which a converted map would resolve differently below a `DMA`, `RO` or
  interrupt-bearing ancestor. The `INTERRUPT` guard stays, so an interrupt register
  keeps writing `triggeredByInterrupt` and no `access`, which `fill` rejects as a
  pair.
- Omits a key whose value would be `0` instead of writing it, since `addInfos`
  rejects a geometry value of `0` and an omitted key falls back to its default,
  which is the value the `0` stood for. A 2D container whose `n_blocks` is `0` has
  no valid emission, since `channels` requires a `pitch`; it is skipped together
  with its channel registers and reported on stderr, as the orphaned-sequence case
  does.

### Alternatives considered

- Caching the resolved values per entry in a `Derived` member written by a
  `derive()` pass: rejected.
  - It needs non-`const` passes and a mutable cache holding state that the tree
    already expresses, and two places would own inheritance rules again, `derive()`
    and `fill`.
  - The address-less register had to copy its parent's register out of the
    catalogue, which only works while the direct parent is a register.
  - `fill` had to carry an `addressSetByParent` flag next to the resolved address
    and the ancestor state.
- Holding the parent as `std::shared_ptr` with `enable_shared_from_this`: rejected,
  the child-to-parent direction closes a cycle and needs a `weak_ptr`, each getter
  would lock it, the children container becomes a map of pointers, and every entry
  needs an allocation - all for a tree that lives inside one function call.
- Making the entries non-movable as well, through `std::unique_ptr` sub-entries or
  a hand-written deserialiser for the children map: rejected, the move assertion
  covers the case that matters, and both alternatives cost a custom parsing path.
- Getters that throw instead of asserting: rejected, a getter has no context to
  name the register in the message, so the rejection lives in `addInfos` and
  `fill`, which know `path()`.
- Requiring a void-typed entry to have an address in its ancestry, as a register
  is located by one: rejected, a void-typed entry carries no data word to locate.
  Taking the ancestor's address instead would make the top-level interrupt
  example of `doc/jmapFormat.dox` an error and force both generators to
  invent an address-bearing parent. Letting it inherit nothing terminates the
  address chain, which leaves its children in the position of top-level entries.
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

## Test plan

All parser test cases go into `tests/executables_src/testJsonMapFileParser.cpp`. Each
case loads `tests/simpleJsonFile.jmap` as the base map, injects its change into
the JSON tree and writes the result to a temporary file, like the existing
selectedBy and map-format-version helpers, so no fault-only fixtures are added.
The load, inject and write sequence lives in one helper that takes the change as
an argument, so a case supplies only its mutation.
The shared fixture is used by the backend test binaries as well, which no case
here affects. Where a case relies on an entry that the fixture already has, it
names that entry below, so a fixture edit that changes what the case inherits
shows up in review. A rejection case expects a `ChimeraTK::logic_error` and
nothing more; no case asserts an error message.

- `tests/executables_src/testJsonMapFileParser.cpp` (modified)
  - `TestAddressInheritanceDocumentedExample`: the documented inheritance
    example in `doc/jmapFormat.dox` - an address-bearing parent on `IO` channel 2
    with one child carrying only a `representation` and one child
    whose `address` gives only `offset`; both children land on the parent's
    channel.
    - Requirement: per-field inheritance of a partial `address`; the example's
      address-less child also inherits `numberOfElements: 2` of the parent,
      which is asserted here.
    - Necessary: the example's root is `APP`, which the fixture already has with
      the children `STATUS`, `LARGE_STATUS`, `SomeTable` and `SomeFloat`, so the
      case replaces that module instead of adding one. `BSP` already has partial
      child addresses (`BSP.VERSION` gives only `offset`, `BSP.SOME_INFO` gives
      `channel` 0 and `offset`), but its channel is 0, so an omitted `channel`
      yields 0 whether inherited or defaulted; the example's parent on channel 2
      is the case that tells the two apart. Its `CONTROL` child, which gives its
      own partial `address`, keeps one element of four bytes, which pins that an
      own `address` stops the geometry inheritance.
  - `TestAddressInheritanceOwnTypeDropsChannel`: a child `address` giving `type`
    and the required `offset` (its `channel` omitted) below a `DMA` address on
    channel 5; the child's own `type` applies and its omitted `channel` falls
    back to `0`.
    - Requirement: per-field inheritance of a partial `address` (a `type` of its
      own drops the inherited `channel`).
    - Necessary: the example above covers the inherited `type`/`channel` pair
      only; this is the only case with a `type` of the entry's own.
  - `TestAddressInheritanceOwnChannelKept`: a child `address` giving `channel`
    and the required `offset` (its `type` omitted) below a `DMA` address on
    channel 5; the inherited `type` applies and the `channel` is the entry's
    own, not the inherited 5.
    - Requirement: per-field inheritance of a partial `address` (an inherited
      `type` brings the channel along).
    - Necessary: the other cases have the `channel` omitted; only this one pins
      that an own `channel` wins over the inherited one.
  - `TestChannelsEntryInheritsAddressBecomesRegister`: an entry with `channels`
    and a `pitch`, neither an own `address` nor a `representation`, below an
    address-bearing entry; it yields a register on that address with one element
    and its `pitch * 8` as the element pitch, its channel slices resolve to that
    address and carry the channel's `bytesPerElement`, and a bit-field `children`
    entry nested below one of the channels, say `bitShift: 8` and `width: 4`,
    resolves there too with a slice of its own.
    - Requirement: register or module, and the inheritance chain (`channels`
      makes a register on the address that resolves for it).
    - Necessary: every `channels` entry in the existing maps carries its own
      `address`; the existing bit-field `children` sit below such channels, so
      an address-less register passing its address to a channel and on to a
      bit-field child is untested.
  - `TestGroupingEntryBelowAddressBecomesRegister`: an address-bearing entry
    with `bytesPerElement: 8` carrying `access: RO` and `numberOfElements: 3`
    whose child gives only `children`, with an address-less grandchild carrying a
    `representation`; the child inherits the address and the access and yields a
    register of its own with a 64-bit fixed-point word, and the grandchild is a
    `READ_ONLY` register on the same address with the parent's
    `numberOfElements`.
    - Requirement: register or module, and the inheritance chain (an entry below
      an address is a register, whatever it gives itself); also the inheritance of
      `access` and `numberOfElements`.
    - Necessary: the existing maps have modules above a register-bearing entry
      (`APP`, `MY_INTC`) and a module without any address (`MY_INTC.SUB0`), but no
      grouping entry below an address with properties to inherit. One level
      suffices, the pass-through is the recursion itself.
  - `TestModuleWithPropertiesRejected`: a true module (no address in its ancestry)
    that gives `access`, a second one that gives `triggeredByInterrupt`, a third
    that gives `numberOfElements`, a fourth that gives `bytesPerElement`, a fifth
    that gives `pitch` and a sixth that gives `doubleBuffering` are rejected; a
    seventh one that carries only `engineeringUnit` and `description` next to
    `children` parses.
    - Requirement: register or module, and the inheritance chain (a module may
      give `children`, `selectedBy`, `engineeringUnit` and `description`; its data
      members are rejected).
    - Necessary: `MY_INTC` in the fixture is a true module, and
      `TestRegisterWithoutAncestorAddressRejected` covers only the
      `representation`; the other members are rejected by their own check, and no
      fixture module carries `engineeringUnit` or `description`.
  - `TestGeometryInheritanceAndOwnSampleWidth`: an address-bearing parent with
    `numberOfElements: 2` and `bytesPerElement: 4` and three address-less children
    carrying a `representation`: the first inherits both values and the parent's
    `elementPitchBits`, the second gives `bytesPerElement: 2`, keeps
    `numberOfElements` 2 and keeps the parent's `elementPitchBits`, the third gives
    `numberOfElements: 1` and keeps its own value even though it equals the
    default.
    - Requirement: inheritance of the register properties (`numberOfElements`,
      `bytesPerElement`); an own value of either overrides the inherited one.
    - Necessary: no existing case has an address-less child below a parent with a
      non-default geometry; both overrides need a case, and the third child pins
      that the value `1` is told from an omitted field.
  - `TestGeometryOf2DChild`: two address-less children of a 2D register that
    carries `pitch: 8` and is given a `representation`: the first takes the 2D
    register's `pitch` as its sample width and `pitch * 8` as its element pitch,
    the second gives an own `bytesPerElement` and takes it as the sample width
    while the stride stays `pitch * 8`.
    - Requirement: inheritance of the register properties (`bytesPerElement`
      below a 2D sample, bit-field children of a channel).
    - Necessary: the 2D geometry is the one case in which the sample width is not
      a `bytesPerElement` of the ancestor, and no existing map has an address-less
      child of a 2D register. The `pitch` is 8, so the inherited sample width
      yields a 64-bit word; a `pitch` the representation cannot express would
      leave the raw type at `none` and hide the rule.
  - `TestChannelsWithOwnRepresentation`: a register with `channels`, a `pitch`,
    `numberOfElements: 2` and `bytesPerElement: 4` that also gives a
    `representation`; it yields one register whose channels are exactly the named
    ones, so the own `representation` adds no channel of its own.
    - Requirement: register or module (an entry with `channels` is a register
      with or without a `representation`).
    - Necessary: no fixture gives both, and `Representation::fill` would append a
      channel to a 2D register.
  - `TestChannelsKeepOwnGeometry`: a child of a parent with `numberOfElements: 2`
    giving `channels` with a `pitch` and no `numberOfElements`; it yields one
    element, so the parent's value does not reach it.
    - Requirement: inheritance of the register properties (`numberOfElements`,
      `bytesPerElement` are inherited only where the whole `address` is omitted
      and the entry gives no `channels`).
    - Necessary: `TestAddressInheritanceDocumentedExample` pins the positive half
      of the rule and its `CONTROL` child the own-`address` half; this case adds
      the `channels` half, which no other case reaches.
  - `TestEmptyEntryNameRejected`: an entry whose key in `children` or
    `addressSpace` is the empty string is rejected.
    - Requirement: register or module, and the inheritance chain (the linking
      pass names every entry).
    - Necessary: the check moves from `addInfos` into `linkTo`, which
      sees the map key, and no existing case feeds an empty key.
  - `TestAddressWithoutOffsetRejected`: a present `address` without `offset` is
    rejected.
    - Requirement: per-field inheritance of a partial `address` (`offset` is
      always the entry's own, as the schema's `address` definition states).
    - Necessary: no existing map has an address without `offset`.
  - `TestSecondaryBufferAddressWithoutOffsetRejected`: a present
    `secondaryBufferAddress` without `offset`, and a `doubleBuffering` object that
    omits `secondaryBufferAddress` altogether, are rejected.
    - Requirement: per-field inheritance of a partial `address`
      (`secondaryBufferAddress`, whose `offset` is its own and required).
    - Necessary: no existing map has a secondary address without `offset`, and
      every existing `doubleBuffering` gives `secondaryBufferAddress`, so both
      injected maps are new: one with the member present without `offset`, one
      with it omitted.
  - `TestSecondaryBufferAddressInheritsTypeAndChannel`: a double-buffered
    register whose primary `address` is `DMA` on channel 1 and whose
    `secondaryBufferAddress` gives only its `offset`; it parses, with the
    secondary on the primary's channel 1.
    - Requirement: per-field inheritance of a partial `address` (a
      `secondaryBufferAddress` inherits an omitted `type`/`channel` from the
      primary `address` of the same entry).
    - Necessary: every `secondaryBufferAddress` in the existing maps gives `type`
      explicitly, and no map puts the primary `address` on a non-zero channel, so
      the inherited pair and the accepted same-BAR outcome are untested.
  - `TestSecondaryBufferAddressOwnTypeDropsChannelRejected`: in the same
    fixture, a `secondaryBufferAddress` giving its own `type: IO` and `offset`
    but omitting `channel` does not take the primary's channel 1 over, so it
    resolves to BAR 0 instead of BAR 14 and is rejected by the same-BAR check.
    - Requirement: per-field inheritance of a partial `address`
      (`secondaryBufferAddress`, the `type`/`channel` tie).
    - Necessary: the case above covers the inherited pair only; this is the only
      case with an own secondary `type`, and the primary sits on channel 1 so the
      inherited channel would put both buffers on one BAR.
  - `TestAccessInheritanceWithOwnAddress`: an address-bearing parent with
    `access: RO`, a `representation`, `engineeringUnit` and `description` and two
    children whose own `address` carries only `offset`; both are `READ_ONLY`. The
    first gives its own `representation` and is not reinterpreted by the parent's;
    the second omits it and gets the defaulted fixed-point word, not the parent's.
    Both leave `engineeringUnit` and `description` empty, so neither is inherited.
    - Requirement: inheritance of the register properties (`access`); the
      parent's `representation`, `engineeringUnit` and `description` stay its
      own.
    - Necessary: no fixture combines an own `address` with an omitted `access`
      below a parent whose access is not the default.
  - `TestTriggeredByInterruptInheritance`: an address-bearing parent with
    `triggeredByInterrupt` and three address-less children carrying a
    `representation`; the one without `access` is an `INTERRUPT` register with
    the parent's interrupt id, the one giving `access: RO` is `READ_ONLY`
    without an interrupt id, and a void-typed one with its own
    `triggeredByInterrupt` keeps that id instead of the parent's.
    - Requirement: inheritance of the register properties
      (`triggeredByInterrupt`), also for a child with an own `address`, which the
      case covers with a fourth child whose `address` gives only `offset`.
    - Necessary: no fixture reaches this rule, since no interrupt register in the
      fixture has address-less children; `DAQ.CTRL` and `DAQ.FD` carry
      `triggeredByInterrupt` but stand alone, and the interrupt registers that use
      `channels` have bit-field children in a reduced struct that never reaches
      `fill`.
  - `TestRegisterWithoutAncestorAddressRejected`: an entry with `channels` and an
    entry with a `representation`, each with no address anywhere in its ancestry,
    are rejected.
    - Requirement: register or module, and the inheritance chain.
    - Necessary: no existing map has a register without an address in its
      ancestry. Both entries reach the same rejection, but they cover the two
      members that make an entry a register without an address.
  - `TestChannelsEntryWithoutPitchRejected`: a `channels` entry without `pitch`
    is rejected, now that an address-less `channels` entry parses.
    - Requirement: inheritance of the register properties (a `channels` entry
      must give `pitch`, and `0` is not a valid geometry value).
    - Necessary: the schema requires `pitch` next to `channels`
      (the schema requires it), the parser does not, and a stride of 0
      would otherwise be accepted silently.
  - `TestZeroGeometryRejected`: a `numberOfElements: 0` and a `bytesPerElement: 0`
    are rejected on an otherwise valid register, a channel's `bytesPerElement: 0`
    on a 2D register, and a `pitch: 0` on that 2D register.
    - Requirement: the geometry members are optional, so `0` is an invalid value.
    - Necessary: `0` means "not set" today, and both generators write it, so the
      new reading needs its own case.
  - `TestPitchWithoutChannelsRejected`: an own `pitch` on a register that gives no
    `channels` is rejected.
    - Requirement: inheritance of the register properties (`pitch` describes a 2D
      sample and is only valid next to `channels`).
    - Necessary: the schema forbids the combination and the
      parser ignores it today.
  - `TestDoubleBufferingWithInheritedAddress`: a `doubleBuffering` register
    without an own `address`, below an address-bearing entry, gets its `BUF0`
    view on the resolved address and its `BUF1` view on the address from
    `secondaryBufferAddress`.
    - Requirement: per-field inheritance of a partial `address`
      (`secondaryBufferAddress` inherits from the resolved primary address) and
      the inheritance chain.
    - Necessary: every existing `doubleBuffering` gives its own `address`, and the
      views are created for every register with the member.
  - `TestAccessAndInterruptRejected`: an entry giving both `access` and
    `triggeredByInterrupt` is rejected.
    - Requirement: inheritance of the register properties (the two are mutually
      exclusive).
    - Necessary: the check moves from the entry branch into `fill`,
      and no existing case reaches it.
  - `TestVoidEntryRejections`: a VOID-typed entry with a `doubleBuffering`, a
    second one with `channels`, a third one with an own `address`, a fourth one with
    an own `numberOfElements`, a fifth one with an own `bytesPerElement`, a sixth
    one with an own `pitch` and a seventh one without `triggeredByInterrupt` are
    rejected.
    - Requirement: register or module, and the inheritance chain (a void-typed
      entry carries no data word, no own address, and gives its own
      `triggeredByInterrupt`).
    - Necessary: the existing VOID entries carry none of the seven and all sit
      below an address-bearing entry, so all seven rejections are untested; the
      `fill` branch that holds them is rewritten by this change.
  - `TestVoidEntryBlocksAddressInheritance`: a VOID-typed entry below an
    address-bearing ancestor is a register with no `bar` and no `address` and with
    `nElements` 0; a child injected below it that gives a `representation` but no
    own `address` is rejected, and a sibling child whose `address` gives only
    `offset` parses. The top-level VOID entries of the interrupt example in
    `doc/jmapFormat.dox` parse as well.
    - Requirement: register or module, and the inheritance chain (a void-typed
      entry carries no address and does not continue the ancestor's).
    - Necessary: the VOID entries of the fixture (`BSP.VOID_INTERRUPT_0` and its
      sibling `BSP.VOID_INTERRUPT_3_0_1`) sit below an address-bearing entry and
      only their `pathName` is asserted today, so that they take no address over
      is untested; no fixture has children below a VOID entry, nor a top-level one.
- Not repeated, already covered: an `address` giving only `offset` with no
  address-bearing ancestor defaults to `IO`/`0` (`tests/selectedByInheritance.jmap`,
  e.g. `APP.OUTPUT_SELECT` below the module `APP`); `selectedBy`
  inheritance from a module stays covered by `TestSelectedByInheritance`, which
  must stay green after the getters carry it; a secondary on the same
  BAR is accepted (`DAQ.CTRL` in `TestGoodMapFileParse`) and a different BAR is
  rejected (`TestDoubleBufferDiffBarThrows`). Those fixtures and tests stay
  untouched. The two tools rest on their existing tests: the round trip of
  `chimeratk-device-to-jmap` stays green, and nothing runs
  `chimeratk-map-to-jmap`, which has no test harness.
