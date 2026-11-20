# CR-018: per-field inheritance of JMAP addresses

Synopsis: The parser's inheritance of the address and the register properties is
incomplete and in places wrong. The change defines the rules for each value and
makes the parser follow them. The documentation and the schema state the result.

Status: IN PROGRESS

## Terms

- JSON field: one member of a JSON object.
- register: an entry with an address, own or inherited, or one that gives a
  `representation` or `channels`.
- module: an entry with neither; it may give `children`, `selectedBy`,
  `engineeringUnit` and `description`.
- 2D register: a register with `channels`.
- void-typed entry: an entry whose `representation` is `void`.
- element: one value a register reads or writes.
- pitch: the byte distance between two elements of the same channel in a 2D register.
- grouping entry: an entry below an address that gives only `children`.

## Requirements

Aspect: inheritance.

- R1: Resolution is per JSON field. Where an entry does not give a value itself,
  the entry takes its parent's value, which the parent resolved in the same way.
- R2: `address`
  - An entry without an `address` takes the parent's whole address, `offset`
    included.
  - An own `address` must give its own `offset`. `offset` is never inherited.
  - An omitted `type` takes the parent's `type` and with it its `channel`.
  - An own `type` drops the inherited `channel`, because a channel means something
    different per `type`. An omitted `channel` is `0` in that case.
  - Without a parent address, `type` and `channel` are `IO` and `0`; `offset` has
    no default.
  - An entry that gives a non-void `representation` or `channels` needs an address,
    own or inherited; without one it is rejected.
- R3: `doubleBuffering`
  - `secondaryBufferAddress` is mandatory, and the second buffer lies on the same
    channel as the register.
  - `secondaryBufferAddress` gives only its own `offset`, which is required.
  - A `type` or `channel` on the `secondaryBufferAddress` is rejected.
- R4: `access` and `triggeredByInterrupt`
  - The entry inherits `access` from the parent unless it gives `access` or
    `triggeredByInterrupt`. This holds whether the entry has an own `address` or
    not.
  - `triggeredByInterrupt` is inherited together with the `access` it implies. An
    own `access` on the entry suppresses this inheritance and keeps the child a
    plain register.
  - An entry cannot specify both `access` and `triggeredByInterrupt`.
- R5: `numberOfElements` and `bytesPerElement`
  - `numberOfElements` and `bytesPerElement` are inherited per value from the
    parent. Inheritance applies only where the entry omits its whole `address` and
    gives no `channels`. The values describe the element the entry is cut from.
  - `0` is rejected for `numberOfElements`, `bytesPerElement` and `pitch`. An
    absent key means "not set"; a `0` does not.
- R6: 2D registers and their channels
  - A `representation` next to `channels` adds no channel; the register's channels
    are exactly those named.
  - The register gives `pitch`, and a `pitch` without `channels` is rejected. The
    register passes neither `bytesPerElement` nor pitch to its children: each child
    takes its own `bytesPerElement` or `4` and advances by its element size.
  - The channels take the register's address; each gives its own `offset` (`0` is
    valid) and its own `bytesPerElement` or `4`.
  - A channel's `bytesPerElement` is the word context of its bit-field `children`.
  - A channel and its bit-field child need a width. A void `representation` on
    either is therefore rejected.
  - A register-level `selectedBy` next to `channels` applies to the channels.
- R7: The pitch is inherited like the other geometry values, so an own
  `bytesPerElement` changes the element size only.
- R8: If a register gives neither `representation` nor `channels`, it gets a
  fixed-point word at its resolved address, with a width equal to its effective
  element size. A grouping entry is also such a register, so the parser emits it
  in addition to its children.
- R9: `selectedBy` is inherited from the parent.
- R10: `representation`, `engineeringUnit` and `description` are never inherited.
- R11: The void-typed exception
  - The search for an address stops at the void-typed entry, so its children are in
    the position of top-level entries.
  - `selectedBy` passes through the void-typed entry. `access` and
    `triggeredByInterrupt` are not taken from above the void-typed entry. The
    void-typed entry gives a `triggeredByInterrupt` of its own, and this value
    reaches its descendants like the value of any other register.
- R12: An entry without a data word
  - A module and a void-typed entry carry no word, so both reject
    `numberOfElements`, `bytesPerElement`, `pitch` and `doubleBuffering`.
  - A module rejects `access` and `triggeredByInterrupt` as well; a void-typed
    entry rejects `channels`.

Aspect: documentation, format version and tools.

- R13: `doc/jmapFormat.dox` and `schemas/jmap.schema.json` state the rules above.
- R14: The format version stays `1.0`, so the rules apply to every file that
  declares it and a map that violates them stops parsing.
- R15: The following maps parse today but resolve differently after the change.
  They are no violations:
  - a register with an own `address` and no `access` below a parent with a
    non-`READ_WRITE` access takes the inherited access and interrupt
  - a child with an own `access` below an interrupt-bearing parent loses the
    inherited interrupt id
  - an own `address` with an omitted `type` below a parent on another channel
    takes the parent's `type`, so the register moves to another `type`/`channel`
    combination
  - an own `numberOfElements` on an address-less child is honoured
  - an entry below an address that gives only `children` becomes a register
  - an address-less child of a 2D register advances by its element size, not by
    the 2D `pitch`
  - a register without a `representation` gets a word of its effective width, not
    32 bits
- R16: `chimeratk-map-to-jmap` emits maps that satisfy the rules above: an `offset`
  in every `address`, an explicit `type`, and an explicit `access` on every
  register except an interrupt register.

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
  - `triggeredByInterrupt` keeps CR-020's optional `InterruptId`, so the getters
    pass a register name and a numeric array on alike.
  - `access` becomes `std::optional<Access>`, and `Access::accessNotSet` is
    deleted. Its value collided with
    `NumericAddressedRegisterInfo::Access::INTERRUPT`; with the sentinel gone, the
    getter expresses the interrupt case with the catalogue's enum.
  - `parent` and `fullPath` are not in the macro's member list, so they are
    neither read from nor written to JSON.
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
  qualified path of the entry. `addInfos` sets `fullPath` before it does anything
  else, from the `name` and `parentName` arguments, with the dotted alt separator,
  as today.
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
  address. Consequently, `isRegister()` inspects the members and not
  `effectiveAddress()`.
- `std::optional<Address> effectiveAddress() const` (new): the resolved address.
  - An own `address` resolves with `Address::resolveTypeAndChannel` to the parent's
    resolved `type`/`channel`; its `offset` is its own and mandatory.
  - Without an own `address` the entry returns the parent's `effectiveAddress()`,
    so the per-field search continues above it, and `std::nullopt` where the parent
    has none.
  - A void-typed entry returns `std::nullopt` whatever the parent gives, so it is
    the one register without a resolved address. It consequently writes no address
    into the register info and passes none on, so the search of a child stops at
    it.
- `NumericAddressedRegisterInfo::Access effectiveAccess() const` (new): the own
  `access`. Otherwise `INTERRUPT` where `triggeredByInterrupt` is present, whether
  a name or an array. Otherwise the parent's value, and `READ_WRITE` at the top.
  The catalogue's enum is the return type because it has an `INTERRUPT` value; the
  own `access` member stays the parser's `Access`, converted here once.
  - A VOID-typed entry inherits neither and takes its own `triggeredByInterrupt`,
    which gives it `INTERRUPT`.
- `std::optional<NumericAddressedRegisterInfo::InterruptId>
  effectiveTriggeredByInterrupt() const` (new): the own declaration. Otherwise it
  is the parent's, where `effectiveAccess()` is `INTERRUPT`. It is empty where the
  entry gives an `access` of its own, which suppresses the inheritance. `fill` writes
  this value into `info.interruptId`, which CR-020 resolves after the parse. An
  own `access` next to an own `triggeredByInterrupt` stays rejected: the tie lives
  in `effectiveAccess()`, and this getter derives from that.
- `size_t effectiveNumberOfElements() const`, `size_t effectiveElementBytes()
  const` and `size_t effectiveElementPitchBits() const` (new): the geometry is
  inherited per JSON field, and only where the entry omits its whole `address`
  and gives no `channels`. Where the entry gives its own value, the getter returns
  it. Where it gives no own value and nothing is inherited, the getter returns `1`
  for the number of elements or `4` for the element size. The pitch is
  `effectiveElementBytes() * 8`.
  - In that case the entry inherits the pitch in bits from the parent. An own
    `bytesPerElement` changes only the element size. A 2D parent gives no pitch, so
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
  selectedBySource() const` (new): the effective selector is the own value, else
  the parent's, for a module and a void-typed entry as well. The source is
  `path()` where the entry declares the selector, else the parent's source, and is
  empty at the top, as the `addInfos` argument is today.
- `Representation`, `engineeringUnit`, `description`, `doubleBuffering` and
  `channels` are never inherited. The class's own members read them directly, so
  they need no getter. `pitch` is an own member too.
- `void fill(NumericAddressedRegisterInfo& info) const` (modified): writes
  `info.pathName` from `path()` and the non-inheritable members, and takes every
  other value from the getters. It holds no inheritance logic, needs no path
  argument, and adds no rejection; the existing read-only `selectedBy` check and
  CR-020's checks for an empty `triggeredByInterrupt` stay here.
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
    descriptor, leaves the address empty, and yields the single channel that
    `fill` emplaces today: bit offset 0, `VOID` type, width 0, fractional bits 0,
    unsigned.
- `void addInfos(NumericAddressedRegisterCatalogue& catalogue)` (modified):
  It rejects an own `address` without `offset` before any `effectiveAddress()`
  call, so `resolve` never reads an absent `offset`. It creates a register for
  `isRegister()` entries through `fill` and recurses into the sub-entries.
  - It is non-`const` only to record the links: it sets `fullPath` of the entry
    and `child.parent = this` of each sub-entry before it recurses, so `parse`
    binds the entries by `auto&`.
  - The `addressSetByParent`, `inheritedSelectedBy` and
    `inheritedSelectedBySource` arguments are gone: the getters reach the parent
    through the link.
  - It rejects every non-void entry where `isRegister()` is true yet
    `effectiveAddress()` is empty. The error message is `Address not set but
    representation or channels given`. This covers a `representation` and `channels`, so the rule
    lives in one place, and `fill` never sees a non-void register without an
    address; a void-typed entry has none by definition and passes.
  - It owns every rejection the change adds, and runs them before it calls
    `fill`, so `fill` never reads a value they guard. The two in
    `DoubleBufferingInfo::fill` stay there, since only `fill` holds the resolved
    primary address.
  - It rejects:
    - a channel without `offset`,
    - a `channels` entry without `pitch`,
    - an own `pitch` on a register without `channels`,
    - an own `access` next to an own `triggeredByInterrupt`,
    - a `numberOfElements`, `bytesPerElement` or `pitch` of `0` on an entry or a
      channel,
    - a void `representation` on a channel or on a bit-field child of one.
  - It rejects a VOID entry that gives `doubleBuffering`, `channels`, an own
    `address`, or an own `numberOfElements`, `bytesPerElement` or `pitch`, because
    such an entry carries no data word. It also rejects a VOID entry
    without an own `triggeredByInterrupt`.
  - It calls `info.computeDataDescriptor()` after `fill`, as today,
    so the descriptor matches the filled channels.
  - It enters every register through `addSliceWithBufferViews`, which adds the
    register itself and creates the `BUF0`/`BUF1` views where `fill` has set
    `info.doubleBuffer`, so no second `catalogue.addRegister` is needed.
  - The `BUF1` view takes its address from `info.doubleBuffer->address`, which
    `DoubleBufferingInfo::fill` wrote from the required secondary `offset`.
  - A module that gives any of `access`, `triggeredByInterrupt`,
    `numberOfElements`, `bytesPerElement`, `pitch` and `doubleBuffering` is
    rejected with `logic_error`.
- `Channel::offset` becomes `std::optional<size_t>`, so an omitted `offset` is
  told from `0`, which stays valid. `addInfos` rejects an omitted `offset` before
  `Channel::fill` and `channelsInOffsetOrder` dereference it.
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
  - An own `type` wins. An omitted `type` takes the inherited `type` together with
    its `channel`. With no inherited address, `type` becomes `IO` and `channel`
    becomes `0`.
  - An own `channel` always wins.
- `static Address resolve(const Address& own, const std::optional<Address>&
  inherited)` (new): resolves an own address to a fully populated `Address` whose
  `offset` is the `offset` of that own address. `inherited` must already be resolved by the
  caller, because `resolveTypeAndChannel` restarts at `IO`/`0` for an inherited
  address with omitted fields. `effectiveAddress()` returns the result, which
  `Address::fill` writes into `info` as today; it reads `*offset`, and its
  `addressTypeNotSet` assertion becomes an assertion that `resolve` ran.

### JsonAddressSpaceEntry::DoubleBufferingInfo (modified) in src/JsonMapFileParser.cc (modified)

- `std::optional<Address> secondaryBufferAddress` (modified): `struct
  SecondAddress` disappears and `Address` is reused, since its `type`/`channel`
  members are what tell a map that still gives them. The whole member is optional,
  so an omitted `secondaryBufferAddress` is told from a present one.
- `void fill(NumericAddressedRegisterInfo& info) const` (modified): writes the
  secondary `offset` into `info.doubleBuffer->address`; the second buffer lies on
  the same channel.
  - It rejects an omitted `secondaryBufferAddress`, one without `offset`, and one
    that gives `type` or `channel`, all with `logic_error`. No comparison remains:
    the parser enforces that both buffers lie on the same channel.

### Format documentation (modified) in doc/jmapFormat.dox (modified)

- `jmap_access`: a register that gives no `access` inherits the parent's access.
  Such a register also inherits the parent's `triggeredByInterrupt`, unless it
  gives an `access` of its own. A register with an own `address` inherits in the same way.
  A void-typed entry inherits neither `access` nor `triggeredByInterrupt`, while
  its own interrupt reaches its descendants like any other register's.
- `doubleBuffering`: `secondaryBufferAddress` is mandatory and gives only its
  `offset`; a `type` or `channel` on it is invalid, and both buffers lie on the
  same channel.
- `jmap_representation`: a register that omits its whole `representation` and
  gives no `channels` gets a fixed-point word of its effective element size, not a
  32-bit word.
- `jmap_elements`: `numberOfElements` and `bytesPerElement` are inherited from the
  parent where the entry omits its whole `address` and gives no `channels`, with
  own values overriding the inherited ones; otherwise they default to `1` and `4`.
  A value of `0` is invalid. A channel gives its own `bytesPerElement` or `4`.
- `jmap_modules`: a module is an entry for which no address resolves on the way up
  to a void entry or the top, and which gives neither `representation` nor
  `channels`.
  A module may give `children`, `selectedBy`, `engineeringUnit` and `description`,
  and `access`, `triggeredByInterrupt`, `numberOfElements`, `bytesPerElement`,
  `pitch` and `doubleBuffering` on it are rejected. An entry with `channels` is a
  register on the address that resolves for it, and one with `representation` or
  `channels` but no such address is rejected.
- `jmap_address_inheritance`, the `type`, `channel` and `offset` bullets of the
  `address` object: a present `address` always gives `offset`; only `type` and
  `channel` may be omitted. Both `type` and `channel` are inherited from the
  parent. If the entry gives its own `type`, the inherited `channel` is dropped and
  an omitted `channel` is `0`. Only where the parent has no address do they default
  to `IO`/`0`.
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

- Writes `type` and `access` explicitly, since both are inherited now.
  `_make_address` omits `"type"` for `IO`, and the serialiser omits a default
  `RW` `access`; below a `DMA`, `RO` or interrupt-bearing parent, a converted map
  would resolve both differently. The `INTERRUPT` guard stays, so an interrupt register
  keeps writing `triggeredByInterrupt` and no `access`, which `fill` rejects as a
  pair.
- Drops `_ensure_void_parents` and the unused `parent_data` parameter: a top-level
  void-typed entry parses without an address-bearing parent, and the synthetic one
  would become a register under the new rules.

### Alternatives considered

- Caching the resolved values per entry in a `Derived` member written by a
  `derive()` pass: rejected.
  - It would need non-`const` passes and a mutable cache duplicating state the tree
    already holds, and both `derive()` and `fill` would encode inheritance rules
    again.
  - The address-less register had to copy its parent's register out of the
    catalogue, which only works while the direct parent is a register.
  - `fill` had to carry an `addressSetByParent` flag next to the resolved address
    and the parent state.
- Threading the resolved values forward through the recursion, in a struct of
  address, access, interrupt id, geometry and selector: rejected. It is a second
  data structure that duplicates what the parent already holds and needs a field
  for every inherited value. With recursive resolution, each rule appears in
  exactly one getter.
- A linking pass that stores the parent pointer before `addInfos`, with a
  `linked` flag and assertions on move and copy: rejected. The descent assigns the
  pointer itself, so no state the assertions guard can arise.
- Holding the parent as `std::shared_ptr` with `enable_shared_from_this`: rejected. The
  child-to-parent direction closes a cycle and needs a `weak_ptr`, each getter
  would lock it, the children container becomes a map of pointers, and every entry
  needs an allocation - all for a tree that lives inside one function call.
- Making the entries non-movable through `std::unique_ptr` sub-entries or a
  hand-written deserialiser for the children map: rejected. Parsing moves each
  entry into its `std::map` node, and both alternatives cost a custom parsing path
  without removing the need for the parent pointer.
- Getters that throw instead of asserting: rejected. A getter has no context to name
  the register in the message, so the rejection lives in `addInfos` and `fill`,
  which know `path()`.
- Requiring a void-typed entry to have an address in its ancestry, as a register
  is located by one: rejected. A void-typed entry carries no data word to locate.
  Taking the parent's address instead would make the top-level interrupt
  example of `doc/jmapFormat.dox` an error and force both generators to
  invent an address-bearing parent. Letting it inherit nothing terminates the
  address chain, which leaves its children in the position of top-level entries.
- Inheriting the `channel` even where the entry gives its own `type`: rejected. A
  channel means something different per `type`, so an inherited channel across a
  type change is meaningless.
- Keeping `type`/`channel` on `secondaryBufferAddress`, as the schema `$ref` to
  `address` suggests: rejected. A secondary on another channel is unsupported
  anyway, so the fields could only restate the same channel.
- Inheriting an omitted `secondaryBufferAddress` `offset` from the register:
  rejected. The second buffer would share the primary's address and the two
  buffers would be indistinguishable.

## Test plan

All parser cases go into `tests/executables_src/testJsonMapFileParser.cpp`. Each
loads `tests/simpleJsonFile.jmap` and applies its mutation through one helper that
injects and writes a temporary file, like the existing selectedBy and
map-format-version helpers. Thus no fault-only fixtures are added. The fixture is
shared with the backend test binaries, which no case affects; a case relying on an
existing entry names it below, so a fixture edit that changes what it inherits
shows up in review. A case's `Requirement:` names the `R` bullets it covers, and
its `Necessary:` states why no other case covers them. A rejection case expects
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
      is the case that tells the two apart. The example's `CONTROL` child, which
      gives its own partial `address`, keeps one element of four bytes, which pins
      that an own `address` stops the geometry inheritance.
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
    - Requirement: R1, R2 (an own `channel` is kept, the `type` inherited).
    - Necessary: the other cases have the `channel` omitted; only this one pins
      that an own `channel` is kept while an omitted `type` still takes the
      parent's `type`.
  - `TestChannelsEntryInheritsAddressBecomesRegister`: an entry with `channels`
    and a `pitch`, neither an own `address` nor a `representation`, below an
    address-bearing entry; it yields a register on that address with one element
    and its `pitch * 8` as the pitch, its channel slices resolve to that
    address and carry the channel's `bytesPerElement`, and a bit-field `children`
    entry nested below one of the channels, say `bitShift: 8` and `width: 4`,
    resolves there too with a slice of its own. A second channel omits its
    `bytesPerElement`. It defaults to `4`, and its bit-field child is cut from that
    4-byte word.
    - Requirement: R6 (a 2D register and its channels).
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
    - Requirement: R8 (a grouping entry is a register), R4, R5.
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
    - Requirement: R12 (a module rejects data members).
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
    - Requirement: R5, R7 (inherited geometry and overrides).
    - Necessary: no existing case has an address-less child below a parent with a
      non-default geometry; both overrides need a case, and the third child pins
      that the value `1` is told from an omitted field.
  - `TestChannelsWithOwnRepresentation`: a register with `channels`, a `pitch`,
    `numberOfElements: 2` and `bytesPerElement: 4` that also gives a
    `representation`; it yields one register whose channels are exactly the named
    ones, so the own `representation` adds no channel of its own.
    - Requirement: R6 (a `representation` next to `channels` adds no channel).
    - Necessary: no fixture gives both, and `Representation::fill` would append a
      channel to a 2D register.
  - `TestVoidChannelRejected`: a channel of a 2D register whose `representation`
    is `void` is rejected, and so is a bit-field child of a channel with one.
    - Requirement: R6 (no void channel or channel child).
    - Necessary: no fixture has a void channel, and `Channel::fill` would emplace a
      VOID channel with the default width of `32` instead of rejecting it.
  - `TestChannelsKeepOwnGeometry`: a child of a parent with `numberOfElements: 2`
    giving `channels` with a `pitch` and no `numberOfElements`; it yields one
    element, so the parent's value does not reach it.
    - Requirement: R5 (no geometry inheritance with `channels`).
    - Necessary: `TestAddressInheritanceDocumentedExample` pins the positive half
      of the rule and the example's `CONTROL` child the own-`address` half; this
      case adds the `channels` half, which no other case reaches.
  - `TestAddressDefaultsWithoutAncestor`: a top-level entry whose `address` gives
    only `offset` resolves to `IO`, channel `0`, at that offset.
    - Requirement: R2 (`IO`/`0` without a parent).
    - Necessary: no case asserts the resolved `type`/`channel` default;
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
      primary channel.
  - `TestAccessInheritanceWithOwnAddress`: an address-bearing parent with
    `access: RO`, a `representation`, `engineeringUnit` and `description` and two
    children whose own `address` carries only `offset`; both are `READ_ONLY`. The
    first gives its own `representation` and is not reinterpreted by the parent's;
    the second omits it and gets the defaulted fixed-point word, not the parent's.
    The two children leave `engineeringUnit` and `description` empty, so these
    fields are not inherited.
    - Requirement: R4 (`access`), R10 (never inherited).
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
  - `TestTriggeredByInterruptInheritsName`: an address-bearing parent whose
    `triggeredByInterrupt` names another interrupt register of the fixture, and an
    address-less child with a `representation`; both resolve to that register's id
    after `finaliseInterruptBookkeeping()`.
    - Requirement: R4 (`triggeredByInterrupt`).
    - Necessary: CR-020 lets the field name a register; the declaration is a
      variant and nothing covers a name inherited from a parent.
  - `TestRegisterWithoutAncestorAddressRejected`: an entry with `channels` and an
    entry with a `representation`, each with no address anywhere in its ancestry,
    are rejected.
    - Requirement: R2 (no address on the way up).
    - Necessary: no existing map has a register without an address in its
      ancestry. Both entries reach the same rejection, but they cover the two
      members that make an entry a register without an address.
  - `TestChannelsEntryWithoutPitchRejected`: a `channels` entry without `pitch`
    is rejected, now that an address-less `channels` entry parses.
    - Requirement: R6 (`channels` requires `pitch`).
    - Necessary: the schema requires `pitch` next to `channels`, the parser does
      not, and a pitch of 0 would otherwise be accepted silently.
  - `TestChannelWithoutOffsetRejected`: a channel of a 2D register with its
    `offset` removed is rejected.
    - Requirement: R6 (a channel gives its `offset`).
    - Necessary: `Channel::offset` is uninitialised today, so no fixture reaches
      the rejection, and the schema's requirement is unenforced by the parser.
  - `TestZeroGeometryRejected`: a `numberOfElements: 0` and a `bytesPerElement: 0`
    are rejected on an otherwise valid register, a channel's `bytesPerElement: 0`
    on a 2D register, and a `pitch: 0` on that 2D register.
    - Requirement: R5 (`0` is no geometry value).
    - Necessary: `0` means "not set" today and the generators can write it, so the
      new reading needs its own case.
  - `TestPitchWithoutChannelsRejected`: an own `pitch` on a register that gives no
    `channels` is rejected.
    - Requirement: R6 (`pitch` only with `channels`).
    - Necessary: the schema forbids the combination and the
      parser ignores it today.
  - `TestDoubleBufferingWithInheritedAddress`: a `doubleBuffering` register
    without an own `address`, below an address-bearing entry, gets its `BUF0`
    view on the resolved address and its `BUF1` view on the address from
    `secondaryBufferAddress`, both on the same channel.
    - Requirement: R3, R2 (a `doubleBuffering` register below an address).
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
    - Requirement: R12 (what a void-typed entry rejects).
    - Necessary: the existing VOID entries carry none of the seven and all sit
      below an address-bearing entry. The missing `triggeredByInterrupt` is
      rejected by `fill` today and moves into `addInfos` with the other six.
  - `TestVoidEntryBlocksAddressInheritance`: a VOID-typed entry below an
    address-bearing parent is a register with `address` `0` and with `nElements`
    0, while that parent sits at a non-zero offset. A child injected
    below the VOID entry that gives a `representation` but no own `address` is
    rejected. A sibling child whose `address` gives only `offset` and which gives
    no `access` parses as an `INTERRUPT` register with the VOID entry's id,
    resolved to `IO` and channel `0`, so it took the `IO` default rather than a
    value from above. That
    child also carries a `selectedBy` declared above the VOID entry, which passes
    through. The top-level VOID entries of the interrupt example in
    `doc/jmapFormat.dox` parse as well.
    - Requirement: R11, R12 (the void exception), R9 (`selectedBy` passes
      through).
    - Necessary: the VOID entries of the fixture (`BSP.VOID_INTERRUPT_0` and its
      sibling `BSP.VOID_INTERRUPT_3_0_1`) sit below an address-bearing entry and
      only their `pathName` is asserted today, so that they take no address over
      is untested; no fixture has children below a VOID entry, nor a top-level one.
- `TestIsrChildWithChannels` and `TestIsrWithChannels` inject `channels` without a
  `pitch`, which the new rejection now catches first. Both tests get a `pitch` so
  they keep testing the ISR-child rule they exist for. `TestIsrDoubleBuffered` gives
  `type`/`channel` in its injected `secondaryBufferAddress`, which the same
  rejection catches first; it keeps only the `offset`.
- The test that asserts the rejection of a second buffer on another channel is
  removed together with its fixture: a secondary cannot name another channel any
  more.
- Fixtures (modified): `tests/simpleJsonFile.jmap`, `tests/muxedDataAccessor.jmap`
  and `tests/groupedDemux.jmap` lose the `type`/`channel` of every
  `secondaryBufferAddress`.
- Not repeated, already covered: `selectedBy` inheritance from a module stays
  covered by `TestSelectedByInheritance`, which must stay green after the getters
  carry it. `chimeratk-device-to-jmap` is unchanged and keeps its round trips;
  nothing runs `chimeratk-map-to-jmap`, which has no test harness, so its new
  output shape stays unverified.
