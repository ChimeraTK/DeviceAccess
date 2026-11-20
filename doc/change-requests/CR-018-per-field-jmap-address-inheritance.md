# CR-018: per-field inheritance of JMAP addresses

Synopsis: The parser's inheritance of the address and the register properties is
incomplete and in places wrong. The change defines the rules per value and makes
the parser follow them; the documentation and the schema state the result.

Status: IN PROGRESS

## Terms

- JSON field: one member of a JSON object.
- register: an entry that describes data, be it one word, several or a 2D block.
- module: an entry that only groups nested entries and describes no data.
- 2D register: a register with `channels`.
- void-typed entry: an entry whose `representation` is `void`.
- element: one value a register reads or writes.
- pitch: the byte distance between two elements of the same channel in a 2D register.
- grouping entry: an entry below an address that gives only `children`.

## Requirements

Aspect: inheritance.

- R1: Resolution is per JSON field: an entry inherits each value it does not give
  itself from its parent, which resolves its own value by the same rule.
- R2: `address`
  - An entry without an `address` takes the parent's whole address, `offset`
    included.
  - An own `address` must give its own `offset`, which is never inherited.
  - An omitted `type` takes the parent's `type` and with it its `channel`.
  - An own `channel` wins. An own `type` drops the inherited `channel`, so an
    omitted `channel` is then `0`, since a channel means something different per
    `type`.
  - Where the parent has no address, `type` and `channel` are `IO` and `0`. An
    `offset` has no default.
- R3: `secondaryBufferAddress` gives only its own `offset`, and both buffers lie on
  the register's BAR. A `doubleBuffering` without `secondaryBufferAddress`, with an
  offset-less one, or with a `type` or `channel` on it is rejected.
- R4: `access` and `triggeredByInterrupt`
  - `access` is inherited from the parent where the entry gives neither `access`
    nor `triggeredByInterrupt`, with an own `address` or without.
  - `triggeredByInterrupt` is inherited together with the `access` it implies,
    unless the entry gives an `access` of its own. Giving `access` is how a child
    stays a plain register.
  - The two are mutually exclusive on one entry.
- R5: `numberOfElements` and `bytesPerElement`: inherited per value from the
  parent, and only where the entry omits its whole `address` and gives no
  `channels`, since they describe the element the entry is cut from. An own value
  of either overrides the inherited one. A 2D parent gives no `bytesPerElement`, so
  its children take their own or `4`.
- R6: The pitch is inherited with them, so an own `bytesPerElement` changes the
  element size only and the pitch stays the inherited one. A 2D parent gives no
  pitch to inherit, so a child of one advances by its element size.
- R7: A channel gives its own `bytesPerElement` or `4` and never inherits one; it is
  the word context of the channel's bit-field `children`.
- R8: `selectedBy`: inherited from the parent. A register-level `selectedBy` next
  to `channels` applies to the register's channels.
- R9: `representation`, `engineeringUnit` and `description` are never inherited.
- R10: The search for an address stops at a void-typed entry, which has no data
  word, so its children are in the position of top-level entries. `selectedBy`
  passes through it like any other entry's; `access` and `triggeredByInterrupt` do
  not, since a void-typed entry gives its own `triggeredByInterrupt` and that value
  reaches its descendants.

Aspect: registers, modules and valid maps.

- R11: `register` or `module`
  - An entry with an address in its ancestry, own or inherited, is a register.
  - An entry that gives a `representation` or `channels` is a register as well,
    which is how a void-typed entry is a register without an own `address`.
  - An entry with neither is a module. It may give `children`, `selectedBy`,
    `engineeringUnit` and `description`, while `access`, `triggeredByInterrupt`,
    `numberOfElements`, `bytesPerElement`, `pitch` and `doubleBuffering` on it are
    rejected.
- R12: A register without a `representation` and without `channels` is a fixed-point
  word of its effective element size at its resolved address. A grouping entry
  below an address is such a register, so it parses next to its children.
- R13: An entry with `channels` is a register on the address that resolves for it,
  with or without a non-void `representation` and with or without an own
  `address`. Its channels take that address, and the bit-field `children` of a
  channel work off the channel's word.
- R14: An entry with a non-void `representation` or with `channels` is rejected where
  no address resolves on the way up to a void entry or the top.
- R15: A void-typed entry carries no data word: an own `address`, `doubleBuffering`,
  `channels`, `numberOfElements`, `bytesPerElement` or `pitch` on it is rejected,
  and so is one without an own `triggeredByInterrupt`. It inherits neither
  `access` nor `triggeredByInterrupt`, and its own `triggeredByInterrupt` reaches
  its descendants like any other register's.
- R16: A channel of a 2D register and a bit-field child of a channel must not have a
  void `representation`: both need a width, so both are rejected.
- R17: The geometry members and the channel `offset`
  - `0` is rejected for `numberOfElements`, `bytesPerElement` and `pitch`, since an
    absent key states "not set" and `0` does not.
  - A `channels` entry gives its `pitch`; `pitch` without `channels` is rejected.
  - Every channel gives its `offset`, since `0` is a valid offset and cannot stand
    for "not set".

Aspect: documentation, format version and tools.

- R18: `doc/jmapFormat.dox` and `schemas/jmap.schema.json` state the rules above.
- R19: The format version stays `1.0`: the rules apply to every file that declares
  it, and a map that violates them stops parsing.
- R20: These maps parse today and resolve differently without being violations:
  - a register with an own `address` and no `access` below a parent whose
    access is not `READ_WRITE` takes the inherited access and interrupt instead
    of `READ_WRITE`
  - a child with an own `access` below an interrupt-bearing parent loses the
    inherited interrupt id
  - an own `address` with an omitted `type` below a parent on another BAR takes
    the parent's `type`, so its `bar` changes
  - an own `numberOfElements` on an address-less child is honoured instead of
    ignored
  - an entry below an address that gives only `children` becomes a register of
    its own
  - an address-less child of a 2D register advances by its element size instead
    of the 2D `pitch`
  - a register without a `representation` gets a word of its effective width
    instead of a 32-bit word
- R21: `chimeratk-map-to-jmap` emits maps that satisfy the rules above: an `offset`
  in every `address`, an explicit `type`, and `access` explicit except on an
  interrupt register, which carries `triggeredByInterrupt` instead.

## Specifications

### JsonAddressSpaceEntry (modified) in src/JsonMapFileParser.cc (modified)

- Data members become private. All inheritance is resolved by `const` getters that
  consult the parent entry. The only non-`const` step is the descent assigning
  each child its parent link. Each rule is stated in one getter only, and no
  resolved value is threaded down the parsing recursion.
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
  - `parent` and `fullPath` are not in the macro's member list, so they are
    neither read from nor written to JSON.
- `size_t Address::bar() const` (new): `channel + (type == DMA ? 13 : 0)`, the one
  place for the formula inside the parser, used by `Address::fill`. The inverse mapping stays in `_make_address` of
  `chimeratk-map-to-jmap` and in the `channel` bullet of `doc/jmapFormat.dox`. It
  reads `type`/`channel` as values, so it asserts that `resolve` ran, like
  `Address::fill`.
- `const JsonAddressSpaceEntry* parent` (new): the direct parent entry, null at
  the top level.
  - The descent assigns it on every child where it recurses, so a getter always
    finds a complete chain. Neither a linking pass nor a "linked" flag is needed.
  - The pointer refers into the tree of entries. A parent owns its sub-entries
    through `std::map` nodes, so it outlives its child, and no pointer leaves the
    tree: `addressSpace` is a local of the parse function and only copies of
    `NumericAddressedRegisterInfo` leave it.
  - `NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT` builds every entry from the
    JSON before the descent starts, and nothing copies a linked entry afterwards,
    so no copy can carry a stale parent.
- `const RegisterPath& path() const` (new): the `fullPath` member, the fully
  qualified path of the entry. `addInfos` sets it from its `name` and `parentName`
  arguments before it does anything else, with the dotted alt separator, as today.
  - It replaces the `name` and `parentName` arguments of `fill`, and is what the
    error messages and `info.pathName` use.
  - It needs no second lookup to name the entry that declared the effective
    selector.
- `std::optional<Address> address` in `JsonAddressSpaceEntry`, so an omitted
  `address` is a `std::nullopt` and an `address: {}` is a present value whose
  fields are all omitted.
- `bool isRegister() const` (new): an entry for which an address resolves, own
  or inherited, or which gives a `representation` or `channels`; an entry with
  none of these is a module. A void-typed entry is thus a register without an
  address, which is why this test reads the members and not `effectiveAddress()`.
- `std::optional<Address> effectiveAddress() const` (new): the resolved address.
  - An own `address` resolves with `Address::resolveTypeAndChannel` to the parent's
    resolved `type`/`channel`; its `offset` is its own and mandatory.
  - Without an own `address` the entry returns the parent's `effectiveAddress()`,
    so the per-field search continues above it, and `std::nullopt` where the parent
    has none.
  - A void-typed entry returns `std::nullopt` whatever the parent gives, so it
    is the one register without a resolved address; it consequently neither
    writes an address into the register info nor passes one on, and the search of
    a child stops at it.
- `NumericAddressedRegisterInfo::Access effectiveAccess() const` (new): the own
  `access`; else `INTERRUPT` where the entry gives its own `triggeredByInterrupt`;
  else the parent's, and `READ_WRITE` at the top. The catalogue's enum is the
  return type because it has an `INTERRUPT` value; the own `access` member stays
  the parser's `Access`, converted here once.
  - A VOID-typed entry inherits neither and takes its own `triggeredByInterrupt`,
    which gives it `INTERRUPT`.
- `std::vector<size_t> effectiveInterruptId() const` (new): the own
  `triggeredByInterrupt`; else the parent's where `effectiveAccess()` is
  `INTERRUPT`; empty where the entry gives an `access` of its own, which
  suppresses the inheritance. An own `access` next to an own
  `triggeredByInterrupt` stays rejected, so the tie lives in `effectiveAccess()`
  and this getter derives from it.
- `size_t effectiveNumberOfElements() const`, `size_t effectiveElementBytes()
  const` and `size_t effectiveElementPitchBits() const` (new): the geometry is
  inherited per JSON field, and only where the entry omits its whole `address`
  and gives no `channels`. Elsewhere each getter returns the entry's own value,
  `1` and `4` where it gives none, and the pitch `effectiveElementBytes() * 8`.
  - Where the entry omits its whole `address` and gives no `channels`, it inherits
    the pitch in bits from the parent, an own `bytesPerElement` changing only the
    element size. A 2D parent gives none, so
    the child's pitch follows its element size.
  - An entry with `channels` uses its own `pitch` in bytes times 8, `addInfos`
    having rejected a `channels` entry without `pitch`.
  - One private predicate serves all three getters, each asking the parent for its
    own value.
  - Bit-field `children` of a channel take the channel's `bytesPerElement` as
    their word context (`doc/jmapFormat.dox`, section `jmap_channel_bitfields`).
    An own `bytesPerElement`, like an own `address` on a channel, is ignored: the
    parser skips every key a definition does not list, and the schema's
    `additionalProperties` forbids both. `addInfos` needs no member for them.
- `std::optional<SelectedBy> effectiveSelectedBy() const` and `RegisterPath
  selectedBySource() const` (new): the own value, else the parent's, for a module
  and a void-typed entry as well; the source is `path()` where the entry declares
  it, else the parent's,
  and empty at the top, as the `addInfos` argument is today.
- `Representation`, `engineeringUnit`, `description`, `doubleBuffering` and
  `channels` are never inherited. The class's own members read them directly, so
  they need no getter. `pitch` is an own member too.
- `void fill(NumericAddressedRegisterInfo& info) const` (modified): writes
  `info.pathName` from `path()` and the non-inheritable members, and takes every
  other value from the getters. It holds no inheritance logic, needs no path
  argument, and adds no rejection; the existing read-only `selectedBy` check stays
  here.
  - It applies the register-level selector through `applyRegisterSelectedBy`,
    whose signature stays as it is, fed with `effectiveSelectedBy()`, where the
    entry gives no `channels` and is no VOID. Next to `channels` the selector
    reaches the channels only, which `Channel::fill` and the slices use as today;
    `selectedBySource()` feeds the read-only message.
  - It writes an own `representation` only where the entry gives no `channels`;
    `Representation::fill` would otherwise append a channel to a 2D register.
  - A register that gives no `representation` and no `channels` gets a fixed-point
    `Representation` whose `width` is the effective element size in bits and whose
    `bytesPerElement` is the effective value, so `bytesPerElement: 8` yields a
    64-bit word.
  - A `representation` without `width` keeps the default of `32` from the member
    initialiser; an omitted one leaves the member at `representationNotSet`. The
    `representationNotSet` recursion in `Representation::fill` goes, since `fill`
    supplies an explicit fixed-point representation for every register.
  - It clears `info.channels` before filling, since the default constructor of
    `NumericAddressedRegisterInfo` already creates a channel; `addInfos` no
    longer clears it.
  - A VOID-typed entry writes `elementPitchBits` 0, `nElements` 0 and the `nodata`
    descriptor, leaves `bar`/`address` empty, and yields the single channel that
    `fill` emplaces today: bit offset 0, `VOID` type, width 0, fractional bits 0,
    unsigned.
- `void addInfos(NumericAddressedRegisterCatalogue& catalogue)` (modified):
  rejects an own `address` without `offset` before any `effectiveAddress()` call,
  so `resolve` never reads an absent `offset`; creates a register for
  `isRegister()` entries through `fill`, and recurses into the sub-entries.
  - It is non-`const` only to record the links: it sets `fullPath` of the entry
    and `child.parent = this` of each sub-entry before it recurses, so `parse`
    binds the entries by `auto&`.
  - The `addressSetByParent`, `inheritedSelectedBy` and
    `inheritedSelectedBySource` arguments are gone: the getters reach the parent
    through the link.
  - It rejects every non-void entry for which `isRegister()` holds and
    `effectiveAddress()` is empty, with `Address not set but representation or
    channels given`. This covers a `representation` and `channels`, so the rule
    lives in one place and `fill` never sees a non-void register without an
    address; a void-typed entry has none by definition and passes.
  - It owns every rejection the change adds, and runs them before it calls
    `fill`, so `fill` never reads a value they guard. The two in
    `DoubleBufferingInfo::fill` stay there, since only `fill` holds the resolved
    primary address.
  - It rejects a channel without `offset`, a `channels` entry without `pitch`, an
    own `pitch` on a register without `channels`, an own `access` next to an own
    `triggeredByInterrupt`, a `numberOfElements`, `bytesPerElement` or `pitch` of
    `0` on an entry or a channel, and a void `representation` on a channel or on a
    bit-field child of one.
  - It rejects a VOID entry with a `doubleBuffering`, `channels`, an own `address`
    or an own `numberOfElements`, `bytesPerElement` or `pitch`, since such an entry
    has no data word, and one without an own `triggeredByInterrupt`.
  - It calls `info.computeDataDescriptor()` after `fill`, as today,
    so the descriptor matches the filled channels.
  - It enters every register through `addSliceWithBufferViews`, which adds the
    register itself and creates the `BUF0`/`BUF1` views where `fill` has set
    `info.doubleBuffer`, so no second `catalogue.addRegister` is needed.
  - The `BUF1` view takes its address from `info.doubleBuffer->address`, which
    `DoubleBufferingInfo::fill` wrote from the required secondary `offset`.
  - A module that gives `access`, `triggeredByInterrupt`, `numberOfElements`,
    `bytesPerElement`, `pitch` or `doubleBuffering` is rejected with
    `logic_error`.
- `Channel::offset` becomes `std::optional<size_t>`, so an omitted `offset` is
  told from `0`, which stays valid. `addInfos` rejects it before `Channel::fill`
  and `channelsInOffsetOrder` dereference it.
- Channel and bit-field slices are created for every register with `channels`,
  at its resolved address, so an address-less `channels` entry gets them too.
  - A channel slice takes the register's resolved address plus the channel's
    `offset`, its `bytesPerElement` and the register's geometry; the bit-field
    `children` of a channel work the same way off the channel's word.

### JsonAddressSpaceEntry::Address (modified) in src/JsonMapFileParser.cc (modified)

- `std::optional<AddressType> type`, `std::optional<size_t> channel` and
  `std::optional<HexValue> offset`. `AddressType::addressTypeNotSet` goes
  with them, since no code needs the sentinel any more.
  - An omitted field deserialises to `std::nullopt`, so a field given in the map
    is told apart from an omitted one. `WITH_DEFAULT` keeps that absence.
- `static std::pair<AddressType, size_t> resolveTypeAndChannel(const
  std::optional<AddressType>& ownType, const std::optional<size_t>& ownChannel,
  const std::optional<Address>& inherited)` (new): the single copy of the
  `type`/`channel` tie, used by `effectiveAddress()`.
  - An own `type` wins; an omitted `type` takes the inherited `type` together
    with its `channel`; with no inherited address it becomes `IO` and the
    `channel` `0`.
  - An own `channel` always wins.
- `static Address resolve(const Address& own, const std::optional<Address>&
  inherited)` (new): resolves an own address to a fully populated `Address`, its
  `offset` taken from the own one and `inherited` already resolved by the caller,
  since `resolveTypeAndChannel` restarts at `IO`/`0` for an inherited address with
  omitted fields. `effectiveAddress()` returns the result, so
  `Address::fill` writes `info.address` and `info.bar` from it and
  reads `*offset`; its `addressTypeNotSet` assertion becomes an assertion that
  `resolve` ran.

### JsonAddressSpaceEntry::DoubleBufferingInfo (modified) in src/JsonMapFileParser.cc (modified)

- `std::optional<Address> secondaryBufferAddress` (modified): `struct
  SecondAddress` disappears and `Address` is reused, since its `type`/`channel`
  members are what tell a map that still gives them. The whole member is optional,
  so an omitted `secondaryBufferAddress` is told from a present one.
- `void fill(NumericAddressedRegisterInfo& info) const` (modified): writes the
  secondary `offset` into `info.doubleBuffer->address`; the second buffer lies on
  the register's BAR.
  - It rejects an omitted `secondaryBufferAddress`, one without `offset`, and one
    that gives `type` or `channel`, all with `logic_error`. The resolves and the
    same-BAR check of the primary path are gone.

### Format documentation (modified) in doc/jmapFormat.dox (modified)

- `jmap_access`: a register that gives no `access` inherits the access of the
  parent, together with its `triggeredByInterrupt` unless the
  entry gives an `access` of its own; a void-typed entry inherits neither, while
  its own interrupt reaches its descendants like any other register's. A register
  with an own `address` inherits as well.
- `doubleBuffering`: `secondaryBufferAddress` is mandatory and gives only its
  `offset`; a `type` or `channel` on it is invalid, and both buffers lie on the
  register's BAR.
- `jmap_representation`: a register that omits its whole `representation` and
  gives no `channels` gets a fixed-point word of its effective element size, not a
  32-bit word.
- `jmap_elements`: `numberOfElements` and `bytesPerElement` are inherited from the
  parent, where the entry omits its whole
  `address` and gives no `channels`, own values overriding them, and default to
  `1`/`4` otherwise. A value of `0` is invalid. A channel gives its own
  `bytesPerElement` or `4`.
- `jmap_modules`: an entry without an address on the way up to a void
  entry or the top that gives neither `representation` nor `channels` is a module.
  A module may give `children`, `selectedBy`, `engineeringUnit` and `description`,
  and `access`, `triggeredByInterrupt`, `numberOfElements`, `bytesPerElement`,
  `pitch` and `doubleBuffering` on it are rejected. An entry with `channels` is a
  register on the address that resolves for it, and one with `representation` or
  `channels` but no such address is rejected.
- `jmap_address_inheritance`, the `type`, `channel` and `offset` bullets of the
  `address` object: a present `address` always gives `offset`; only `type` and
  `channel` may be omitted. Both inherit from the parent, and a `type` of its own
  drops the inherited `channel`, so an omitted `channel` is `0` then. Only where the
  parent has no address do they default to `IO`/`0`.
- `jmap_channel_bitfields`: the bit-field `children` of a channel take the
  channel's `bytesPerElement` as their word context.
- `jmap_channel_selection`: neither a channel nor a bit-field child of one may
  have a void `representation`.
- The void-typed entry forbids an own `address` and takes none from its ancestry
  either, so the address chain below it starts afresh; a `doubleBuffering`,
  `channels` or an own geometry on it is rejected. The interrupt example stays as
  it is, its top-level VOID entries having no address.
- The inheritance example states that its `STATUS` child inherits
  `numberOfElements: 2` of `APP`, instead of claiming an identical outcome to the
  example above it.

### JSON schema (modified) in schemas/jmap.schema.json (modified)

- `addressSpaceEntry`: the register and module classification of `jmap_modules`
  above, as description only, since a per-entry schema cannot see whether an
  address or a void entry sits above the entry.
- `numberOfElements` and the `bytesPerElement` definition: the inheritance and
  defaults of `jmap_elements` above, plus `minimum: 1` on both, which covers the
  register member and the channel's; `pitch` already has it.
- `address`: states that only `type` and `channel` are inherited, `offset` being
  the entry's own.
- The 2D-register block drops its `selectedBy: false` and the "must not combine"
  description, since the register-level selector is the default for the channels.
- The `type` and `channel` fields: the inheritance of the `address` bullets
  above.
- `width` and the `representation` description: the defaults of
  `jmap_representation` above.
- `access` and `triggeredByInterrupt`: state the inheritance.
- `channel` and `channelChild`: a `void` `representation` is rejected, since both
  need a width. The channel-child definition also states that it takes the
  channel's `bytesPerElement`; `additionalProperties: false` already rejects an own
  one.
- The void-typed entry: the description says that it forbids an
  own `address` and inherits none, so the address chain below it starts afresh,
  the latter being no constraint a per-entry schema can express;
  `doubleBuffering`, `channels`, the geometry members and an own `address` are
  rejected as new constraints in the `allOf` block.
- `doubleBuffering` gets a `required` list naming `secondaryBufferAddress`, which
  the definition does not have today. `secondaryBufferAddress` gets its own
  definition instead of the `$ref` to `#/definitions/address`: it carries only the
  required `offset` and rejects `type`/`channel` through
  `additionalProperties`.

### JsonMapFileParser::parse (modified) in src/JsonMapFileParser.cc (modified)

- Rejects the maps that the validity rules of the requirements exclude, without
  a format version bump: its version check stays untouched, while `addInfos` and
  `fill` own the individual rejections specified above.
- Calls `addInfos` for every top-level entry straight after parsing. That loop and
  the recursion in `addInfos` bind their entries by `auto&` instead of `const
  auto&`, since `addInfos` assigns the parent link of the sub-entries.

### chimeratk-map-to-jmap (modified) in tools/chimeratk-map-to-jmap (modified)

- Writes `type` and `access` explicitly, since both are inherited now:
  `_make_address` omits `"type"` for `IO` and the serialiser omits a default `RW`
  `access`, which a converted map would resolve differently below a `DMA`, `RO` or
  interrupt-bearing parent. The `INTERRUPT` guard stays, so an interrupt register
  keeps writing `triggeredByInterrupt` and no `access`, which `fill` rejects as a
  pair.
- Drops `_ensure_void_parents` and the unused `parent_data` parameter: a top-level
  void-typed entry parses without an address-bearing parent, and the synthetic one
  would become a register under the new rules.

### Alternatives considered

- Caching the resolved values per entry in a `Derived` member written by a
  `derive()` pass: rejected.
  - It needs non-`const` passes and a mutable cache holding state that the tree
    already expresses, and two places would own inheritance rules again, `derive()`
    and `fill`.
  - The address-less register had to copy its parent's register out of the
    catalogue, which only works while the direct parent is a register.
  - `fill` had to carry an `addressSetByParent` flag next to the resolved address
    and the parent state.
- Threading the resolved values forward through the recursion, in a struct of
  address, access, interrupt id, geometry and selector: rejected, it is a second
  data structure that duplicates what the parent already holds and needs a field
  for every inherited value. Recursive resolution keeps each rule in one getter.
- A linking pass that stores the parent pointer before `addInfos`, with a
  `linked` flag and assertions on move and copy: rejected, the descent assigns
  the pointer itself, so no state the assertions guard can arise.
- Holding the parent as `std::shared_ptr` with `enable_shared_from_this`: rejected,
  the child-to-parent direction closes a cycle and needs a `weak_ptr`, each getter
  would lock it, the children container becomes a map of pointers, and every entry
  needs an allocation - all for a tree that lives inside one function call.
- Making the entries non-movable through `std::unique_ptr` sub-entries or a
  hand-written deserialiser for the children map: rejected, parsing moves each
  entry into its `std::map` node, and both alternatives cost a custom parsing
  path without removing the need for the parent pointer.
- Getters that throw instead of asserting: rejected, a getter has no context to
  name the register in the message, so the rejection lives in `addInfos` and
  `fill`, which know `path()`.
- Requiring a void-typed entry to have an address in its ancestry, as a register
  is located by one: rejected, a void-typed entry carries no data word to locate.
  Taking the parent's address instead would make the top-level interrupt
  example of `doc/jmapFormat.dox` an error and force both generators to
  invent an address-bearing parent. Letting it inherit nothing terminates the
  address chain, which leaves its children in the position of top-level entries.
- Inheriting the `channel` even where the entry gives its own `type`: rejected,
  a channel means something different per `type`, so an inherited channel across
  a type change is meaningless.
- Keeping `type`/`channel` on `secondaryBufferAddress`, as the schema `$ref` to
  `address` suggests: rejected, a secondary on another BAR is unsupported anyway,
  so the fields could only restate the primary's BAR.
- Inheriting an omitted `secondaryBufferAddress` `offset` from the register:
  rejected, the second buffer would share the primary's address and the two
  buffers would be indistinguishable.

## Test plan

All parser cases go into `tests/executables_src/testJsonMapFileParser.cpp`. Each
loads `tests/simpleJsonFile.jmap`, applies its mutation through one helper that
injects and writes a temporary file, like the existing selectedBy and
map-format-version helpers, so no fault-only fixtures are added. The fixture is
shared with the backend test binaries, which no case affects; a case relying on an
existing entry names it below, so a fixture edit that changes what it inherits
shows up in review. A case's `Requirement:` names the `R` bullets it covers, its
`Necessary:` why no other case covers them. A rejection case expects
`ChimeraTK::logic_error` and nothing more.

- `tests/executables_src/testJsonMapFileParser.cpp` (modified)
  - `TestAddressInheritanceDocumentedExample`: the documented inheritance
    example in `doc/jmapFormat.dox` - an address-bearing parent on `IO` channel 2
    at a non-zero `offset`, with one child carrying only a `representation` and
    one child whose `address` gives its own `offset`; both children take the
    parent's `type` and `channel`, the first its `offset` as well.
    - Requirement: R2 (inherited `type`/`channel`), R5 (inherited geometry).
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
    - Requirement: R2 (an own `type` drops the inherited `channel`).
    - Necessary: the example above covers the inherited `type`/`channel` pair
      only; this is the only case with a `type` of the entry's own.
  - `TestAddressInheritanceOwnChannelKept`: a child `address` giving `channel`
    and the required `offset` (its `type` omitted) below a `DMA` address on
    channel 5; the inherited `type` applies and the `channel` is the entry's
    own, not the inherited 5.
    - Requirement: R2 (an own `channel` wins).
    - Necessary: the other cases have the `channel` omitted; only this one pins
      that an own `channel` wins over the inherited one.
  - `TestChannelsEntryInheritsAddressBecomesRegister`: an entry with `channels`
    and a `pitch`, neither an own `address` nor a `representation`, below an
    address-bearing entry; it yields a register on that address with one element
    and its `pitch * 8` as the pitch, its channel slices resolve to that
    address and carry the channel's `bytesPerElement`, and a bit-field `children`
    entry nested below one of the channels, say `bitShift: 8` and `width: 4`,
    resolves there too with a slice of its own. A second channel omits its
    `bytesPerElement`, so it defaults to `4`, and its bit-field child is cut from
    that 4-byte word.
    - Requirement: R13 (`channels` makes a register), R7 (`bytesPerElement` of a
      channel).
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
    - Requirement: R11, R12 (an entry below an address is a register), R4, R5.
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
    - Requirement: R11 (what a module may give).
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
    - Requirement: R5, R6 (inherited geometry and overrides).
    - Necessary: no existing case has an address-less child below a parent with a
      non-default geometry; both overrides need a case, and the third child pins
      that the value `1` is told from an omitted field.
  - `TestChannelsWithOwnRepresentation`: a register with `channels`, a `pitch`,
    `numberOfElements: 2` and `bytesPerElement: 4` that also gives a
    `representation`; it yields one register whose channels are exactly the named
    ones, so the own `representation` adds no channel of its own.
    - Requirement: R13 (a `channels` entry may give a `representation`).
    - Necessary: no fixture gives both, and `Representation::fill` would append a
      channel to a 2D register.
  - `TestVoidChannelRejected`: a channel of a 2D register whose `representation`
    is `void` is rejected, and so is a bit-field child of a channel with one.
    - Requirement: R16 (no void channel or channel child).
    - Necessary: no fixture has a void channel, and `Channel::fill` would emplace a
      VOID channel with the default width of `32` instead of rejecting it.
  - `TestChannelsKeepOwnGeometry`: a child of a parent with `numberOfElements: 2`
    giving `channels` with a `pitch` and no `numberOfElements`; it yields one
    element, so the parent's value does not reach it.
    - Requirement: R5 (no geometry inheritance with `channels`).
    - Necessary: `TestAddressInheritanceDocumentedExample` pins the positive half
      of the rule and its `CONTROL` child the own-`address` half; this case adds
      the `channels` half, which no other case reaches.
  - `TestAddressDefaultsWithoutAncestor`: a top-level entry whose `address` gives
    only `offset` resolves to `bar` `0` at that offset.
    - Requirement: R2 (`IO`/`0` without a parent).
    - Necessary: no case asserts `bar` or `address` for that default;
      `TestSelectedByInheritance` asserts selector values only.
  - `TestAddressWithoutOffsetRejected`: a present `address` without `offset` is
    rejected.
    - Requirement: R2 (`offset` is never inherited).
    - Necessary: no existing map has an address without `offset`.
  - `TestSecondaryBufferAddressWithoutOffsetRejected`: a present
    `secondaryBufferAddress` without `offset`, and a `doubleBuffering` object that
    omits `secondaryBufferAddress` altogether, are rejected.
    - Requirement: R3 (`offset` is required).
    - Necessary: no existing map has a secondary address without `offset`, and
      every existing `doubleBuffering` gives `secondaryBufferAddress`, so both
      injected maps are new: one with the member present without `offset`, one
      with it omitted.
  - `TestSecondaryBufferAddressWithTypeRejected`: a double-buffered register whose
    primary `address` is `DMA` on channel 1 and whose `secondaryBufferAddress`
    gives its own `type: DMA` next to the required `offset` is rejected, and a
    second mutation giving a `channel` is rejected too.
    - Requirement: R3 (`type`/`channel` are invalid).
    - Necessary: every existing map gives a `type` there, so both mutations are
      new; without the rejection the value would silently have to match the
      primary BAR.
  - `TestAccessInheritanceWithOwnAddress`: an address-bearing parent with
    `access: RO`, a `representation`, `engineeringUnit` and `description` and two
    children whose own `address` carries only `offset`; both are `READ_ONLY`. The
    first gives its own `representation` and is not reinterpreted by the parent's;
    the second omits it and gets the defaulted fixed-point word, not the parent's.
    Both leave `engineeringUnit` and `description` empty, so neither is inherited.
    - Requirement: R4 (`access`), R9 (never inherited).
    - Necessary: no fixture combines an own `address` with an omitted `access`
      below a parent whose access is not the default.
  - `TestTriggeredByInterruptInheritance`: an address-bearing parent with
    `triggeredByInterrupt` and four children; the address-less one without
    `access` is an `INTERRUPT` register with the parent's interrupt id, the
    address-less one giving `access: RO` is `READ_ONLY` without an interrupt id,
    the one whose own `address` gives only `offset` is `INTERRUPT` as well, and a
    void-typed child with its own `triggeredByInterrupt` keeps that id instead of
    the parent's.
    - Requirement: R4 (`triggeredByInterrupt`).
    - Necessary: no fixture reaches this rule, since no interrupt register in the
      fixture has address-less children; `DAQ.CTRL` and `DAQ.FD` carry
      `triggeredByInterrupt` but stand alone, and the interrupt registers that use
      `channels` have bit-field children in a reduced struct that never reaches
      `fill`.
  - `TestRegisterWithoutAncestorAddressRejected`: an entry with `channels` and an
    entry with a `representation`, each with no address anywhere in its ancestry,
    are rejected.
    - Requirement: R14 (no address on the way up).
    - Necessary: no existing map has a register without an address in its
      ancestry. Both entries reach the same rejection, but they cover the two
      members that make an entry a register without an address.
  - `TestChannelsEntryWithoutPitchRejected`: a `channels` entry without `pitch`
    is rejected, now that an address-less `channels` entry parses.
    - Requirement: R17 (`channels` requires `pitch`).
    - Necessary: the schema requires `pitch` next to `channels`, the parser does
      not, and a pitch of 0 would otherwise be accepted silently.
  - `TestChannelWithoutOffsetRejected`: a channel of a 2D register with its
    `offset` removed is rejected.
    - Requirement: R17 (a channel gives its `offset`).
    - Necessary: `Channel::offset` is uninitialised today, so no fixture reaches
      the rejection, and the schema's requirement is unenforced by the parser.
  - `TestZeroGeometryRejected`: a `numberOfElements: 0` and a `bytesPerElement: 0`
    are rejected on an otherwise valid register, a channel's `bytesPerElement: 0`
    on a 2D register, and a `pitch: 0` on that 2D register.
    - Requirement: R17 (`0` is no geometry value).
    - Necessary: `0` means "not set" today and the generators can write it, so the
      new reading needs its own case.
  - `TestPitchWithoutChannelsRejected`: an own `pitch` on a register that gives no
    `channels` is rejected.
    - Requirement: R17 (`pitch` only with `channels`).
    - Necessary: the schema forbids the combination and the
      parser ignores it today.
  - `TestDoubleBufferingWithInheritedAddress`: a `doubleBuffering` register
    without an own `address`, below an address-bearing entry, gets its `BUF0`
    view on the resolved address and its `BUF1` view on the address from
    `secondaryBufferAddress`, both on the register's `bar`.
    - Requirement: R3, R11 (a `doubleBuffering` register below an address).
    - Necessary: every existing `doubleBuffering` gives its own `address`, so an
      inherited one with its views is untested.
  - `TestAccessAndInterruptRejected`: an entry giving both `access` and
    `triggeredByInterrupt` is rejected.
    - Requirement: R4 (mutually exclusive).
    - Necessary: the check moves from the entry branch into `addInfos`, and no
      existing case reaches it.
  - `TestVoidEntryRejections`: a VOID-typed entry with a `doubleBuffering`, a
    second one with `channels`, a third one with an own `address`, a fourth one with
    an own `numberOfElements`, a fifth one with an own `bytesPerElement`, a sixth
    one with an own `pitch` and a seventh one without `triggeredByInterrupt` are
    rejected.
    - Requirement: R15 (what a void-typed entry rejects).
    - Necessary: the existing VOID entries carry none of the seven and all sit
      below an address-bearing entry. The missing `triggeredByInterrupt` is
      rejected by `fill` today and moves into `addInfos` with the other six.
  - `TestVoidEntryBlocksAddressInheritance`: a VOID-typed entry below an
    address-bearing parent is a register with `bar` and `address` `0`, while that
    parent sits at a non-zero offset, and with `nElements` 0; a child injected
    below it that gives a `representation` but no own `address` is rejected, and a
    sibling child whose `address` gives only `offset` and which gives no `access`
    parses as an `INTERRUPT` register with the VOID entry's id and with `bar` `0`,
    so it took the `IO` default rather than a value from above; it also carries a
    `selectedBy` declared above the VOID entry, which passes through. The top-level
    VOID entries of the interrupt example in `doc/jmapFormat.dox` parse as well.
    - Requirement: R10, R15 (the barrier), R8 (`selectedBy` passes through).
    - Necessary: the VOID entries of the fixture (`BSP.VOID_INTERRUPT_0` and its
      sibling `BSP.VOID_INTERRUPT_3_0_1`) sit below an address-bearing entry and
      only their `pathName` is asserted today, so that they take no address over
      is untested; no fixture has children below a VOID entry, nor a top-level one.
- `TestIsrChildWithChannels` and `TestIsrWithChannels` inject `channels` without a
  `pitch`, which the new rejection now catches first. They get a `pitch` so they
  keep testing the ISR-child rule they exist for. `TestIsrDoubleBuffered` gives
  `type`/`channel` in its injected `secondaryBufferAddress`, which the same
  rejection catches first; it keeps only the `offset`.
- `TestDoubleBufferDiffBarThrows` is removed with `tests/simpleJsonFile.diffBar.jmap`:
  a secondary cannot name another BAR any more.
- Fixtures (modified): `tests/simpleJsonFile.jmap`, `tests/muxedDataAccessor.jmap`
  and `tests/groupedDemux.jmap` lose the `type`/`channel` of every
  `secondaryBufferAddress`.
- Not repeated, already covered: `selectedBy` inheritance from a module stays
  covered by `TestSelectedByInheritance`, which must stay green after the getters
  carry it. `chimeratk-device-to-jmap` is unchanged and keeps its round trips;
  nothing runs `chimeratk-map-to-jmap`, which has no test harness, so its new
  output shape stays unverified.
