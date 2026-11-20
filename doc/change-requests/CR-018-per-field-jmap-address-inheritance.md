# CR-018: per-field inheritance of JMAP addresses

Synopsis: The parser's inheritance of the address and the register properties is
incomplete and in places wrong. The change defines the rules for each value and
makes the parser follow them. The documentation and the schema state the result.

Status: IN PROGRESS

## Terms

- JSON field: one member of a JSON object.
- entry: an object in `addressSpace` or in a `children` object.
- parent: the entry whose `children` hold this entry; a top-level entry has none.
- inherit: take the parent's value of the same field. A parent that gives no value
  has inherited its own in the same way.
- register: an entry that is no module.
- module: an entry that resolves no address on the way up to a void-typed entry or
  the top, and that gives neither `representation` nor `channels`; it may give
  `children`, `selectedBy`, `engineeringUnit` and `description`.
- 2D register: a register with `channels`.
- void-typed entry: an entry whose `representation` is `void`.
- element: one value a register reads or writes.
- element size: the entry's `bytesPerElement`, in bytes; its width is the same value
  in bits. A channel's element size is its own `bytesPerElement`.
- register-level: given on the entry itself, not on one of its channels.
- pitch: the byte distance between two consecutive elements of a register.
- grouping entry: an entry below an address that gives only `children`.
- bit field: the `children` of a channel, each giving a `representation` with
  `bitShift` and `width` relative to the channel's element.
- geometry: `numberOfElements`, `bytesPerElement` and `pitch`.

## Requirements

Aspect: inheritance.

- R1: `address`
  - An omitted `address` is inherited as a whole, `offset` included.
  - An own `address` must give `offset`; the remaining bullets govern its other
    fields.
  - `type` is inherited.
  - `channel` is inherited unless the entry gives `type`; then an omitted `channel`
    is `0`, since a channel means something different per `type`.
  - Without a parent address, `type` is `IO` and `channel` is `0`.
  - An entry that gives a non-void `representation` or `channels` must have an
    address, own or inherited.
- R2: `doubleBuffering`
  - `secondaryBufferAddress` is mandatory and gives only its own `offset`, which
    is required.
  - A `type` or `channel` on it is rejected, and both buffers lie on the same
    channel.
- R3: `access` and `triggeredByInterrupt`
  - `access` is inherited unless the entry gives `access` or `triggeredByInterrupt`.
  - `triggeredByInterrupt` is inherited together with the `access` it implies.
  - An own `access` suppresses that inheritance and keeps the child a plain
    register.
  - An entry cannot specify both `access` and `triggeredByInterrupt`.
- R4: `numberOfElements` and `bytesPerElement`
  - Inherited only when the whole `address` is omitted and no `channels` are
    given; otherwise the entry uses its own values, `1` and `4` by default.
  - `0` is rejected for `numberOfElements`, `bytesPerElement` and `pitch`.
- R5: 2D registers and their channels
  - A register-level `representation` and `channels` are mutually exclusive, so
    either next to the other is rejected.
  - A `pitch` is given exactly where `channels` are, one rule covering both
    directions: `channels` without `pitch` is rejected, and so is `pitch` without
    `channels`.
  - A 2D register has no `children`; `children` next to `channels` is rejected.
  - Each channel gives its own `offset`, `0` included. Its `bytesPerElement` is
    optional, never inherited, and `4` where omitted.
  - A channel without a `representation` gets a fixed-point word of its own element
    size, as a register does under R7.
  - The `children` of a channel are bit fields of its element. A bit field gives
    only a `representation`; its address, element size and pitch are the
    channel's, so any other member on it is rejected.
  - A void `representation` on a channel or a bit field is rejected.
  - A register-level `selectedBy` next to `channels` applies to the channels.
- R6: `pitch` is inherited together with the geometry. An own `bytesPerElement`
  changes the element size only.
- R7: If a register gives neither `representation` nor `channels`, it gets a
  fixed-point word at its resolved address, with a width equal to its element size
  in bits; a `representation` that omits `width` takes the same width. A grouping
  entry is also such a register, so the parser emits it in addition to its
  children.
- R8: `selectedBy`
  - `selectedBy` is inherited.
  - An inherited `selectedBy` is never overridden: an entry below one that declares
    `selectedBy` must not give its own.
  - A channel and a bit field take the register's effective selector and declare
    none of their own.
- R9: `representation`, `engineeringUnit`, `description` and `channels` are never
  inherited.
- R10: A void-typed entry
  - It carries no data word, so `channels`, an own `address`, `numberOfElements`,
    `bytesPerElement` and `doubleBuffering` are rejected.
  - It must give its own `triggeredByInterrupt`.
  - It has no `selectedBy` of its own, while an inherited one passes through it.
  - It inherits no address, so its children are in the position of top-level
    entries.
  - Its own `triggeredByInterrupt` reaches its descendants like the value of any
    other register.
- R11: A module
  - It carries no data word, so `numberOfElements`, `bytesPerElement` and
    `doubleBuffering` are rejected, and `access` and `triggeredByInterrupt` as
    well.

Aspect: documentation, format version and tools.

- R12: `doc/jmapFormat.dox` and `schemas/jmap.schema.json` state the rules above.
- R13: The format version stays `1.0`, so the rules apply to every file that
  declares it. A map that violates them stops parsing, among them one that
  - gives an own `selectedBy` below an entry that declares one,
  - gives `type` or `channel` on a `secondaryBufferAddress`,
  - gives `channels` and a register-level `representation` together,
  - gives a data member on a module,
  - gives `pitch` without `channels`,
  - gives `numberOfElements`, `bytesPerElement` or `pitch` as `0`, or
  - gives another member than `representation` on a bit field.
- R14: The following maps parse today but resolve differently after the change.
  These differences are accepted, so no migration follows:
  - a register with its own `address` and no `access` below a parent with a
    non-`READ_WRITE` access takes the inherited access and interrupt
  - a child with its own `access` below an interrupt-bearing parent loses the
    inherited interrupt id
  - an own `address` with an omitted `type` below a parent on another channel
    takes the parent's `type`, so the register moves to another `type`/`channel`
    combination
  - an own `numberOfElements` on an address-less child is honoured
  - an entry below an address that gives only `children` becomes a register
  - a register without a `representation` gets a word of its effective width, not
    32 bits
- R15: `chimeratk-map-to-jmap` emits maps that satisfy the rules above: an `offset`
  in every `address`, an explicit `type`, and an explicit `access` on every
  register except an interrupt register.
## Specifications

The parse runs in three steps, kept apart. The JSON deserialises into the entry
objects, then `validate()` rejects every invalid entry in one place, and only then
`fill()` copies the data into the register info, taking each value from a getter.
The getters resolve inheritance and never reject: they assume valid entries, which
`validate()` has established. The parser rejects a defined key in an invalid place
and ignores a key that no definition lists; the schema forbids both. CR-020 is on
master: a `triggeredByInterrupt` may name another register, which is resolved
after the parse.

### JsonAddressSpaceEntry (modified) in src/JsonMapFileParser.cc (modified)

- `const JsonAddressSpaceEntry* _parent` (new): the direct parent entry, and
  `nullptr` at the top level.
  - The descent assigns it on every child where it recurses, so a getter always
    finds a complete chain. Neither a linking pass nor a "linked" flag is needed.
  - The pointer refers into the tree of entries. A parent owns its sub-entries
    through `std::map` nodes, so it outlives its child, and no pointer leaves the
    tree: `addressSpace` is a local of the parse function and only copies of
    `NumericAddressedRegisterInfo` leave it.
  - `NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT` builds every entry from the
    JSON before the descent starts, and nothing copies a linked entry afterwards,
    so no copy can carry a stale `_parent`.
- Data members become private and carry a leading underscore, and every getter
  starts with `get`, so a member and its getter cannot share a name. All
  inheritance is resolved by `const` getters that consult `_parent`. The only
  non-`const` step is the descent assigning each child its parent link. Each rule
  is stated in one getter only, no resolved value is threaded down the parsing
  recursion, and no getter rejects.
  - The class body maps the JSON keys itself, since
    `NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT` derives a key from the member
    name, which carries a leading underscore: `to_json` writes and `from_json`
    reads each key by name, an absent key
    keeping the default of the macro's `value(key, default)`.
  - `_numberOfElements`, `_bytesPerElement` and `_pitch` become
    `std::optional<size_t>`, so an own value is told from an omitted one and a
    module can reject them.
  - `_triggeredByInterrupt` keeps CR-020's optional `InterruptId`, so the getters
    pass a register name and a numeric array on alike.
  - `_access` becomes `std::optional<Access>`, and `Access::accessNotSet` is
    deleted. Its value collided with
    `NumericAddressedRegisterInfo::Access::INTERRUPT`; with the sentinel gone, the
    getter expresses the interrupt case with the catalogue's enum.
  - `_parent` and `_fullPath` are not among the keys, so they are neither read
    from nor written to JSON.
  - A value the map does not give is `std::nullopt` where the member is optional
    and `representationNotSet` for the `representation`; `0` never stands for an
    absent value.
- `const RegisterPath& getPath() const` (new): `_fullPath`, the fully qualified
  path of the entry. `_fullPath` (new) is set by `addInfos` before it does anything
  else, from the `name` and `parentName` arguments, with the dotted alt separator,
  as today.
  - It replaces the `name` and `parentName` arguments of `fill`, and is what the
    error messages and `info.pathName` use.
  - It needs no second lookup to name the entry that declared the effective
    selector.
- `std::optional<Address> _address` in `JsonAddressSpaceEntry`, so an omitted
  `address` is a `std::nullopt` and an `address: {}` is a present value whose
  fields are all omitted.
- `bool isRegister() const` (new): true where the entry gives a `representation`
  (a void one included), gives `channels`, or has a `getAddress()`. An
  entry with none of the three is a module.
- `std::optional<Address> getAddress() const` (new): the resolved address.
  - An own `address` resolves with `Address::resolveTypeAndChannel` to the parent's
    resolved `type`/`channel`; its `offset` is its own and mandatory.
  - Without an own `address` the entry returns the parent's `getAddress()`,
    so the per-field search continues above it, and `std::nullopt` where the parent
    has none.
  - A void-typed entry returns `std::nullopt` whatever the parent gives, so it is
    the one register without a resolved address. It consequently writes no address
    into the register info and passes none on, so the search of a child stops at
    it.
- `NumericAddressedRegisterInfo::Access getAccess() const` (new): the own
  `access`. Otherwise `INTERRUPT` where `triggeredByInterrupt` is present, whether
  a name or an array. Otherwise the parent's value, and `READ_WRITE` at the top.
  The catalogue's enum is the return type because it has an `INTERRUPT` value; the
  own `access` member stays the parser's `Access`, converted here once.
  - A VOID-typed entry inherits neither and takes its own `triggeredByInterrupt`,
    which gives it `INTERRUPT`.
- `std::optional<NumericAddressedRegisterInfo::InterruptId>
  getTriggeredByInterrupt() const` (new): the own declaration. Otherwise,
  where `getAccess()` is `INTERRUPT`, the parent's declaration. Empty where
  the entry gives an `access` of its own, which suppresses the inheritance. `fill`
  writes this value into `info.interruptId`, which CR-020 resolves after the parse.
  An own `access` next to an own `triggeredByInterrupt` stays rejected: the tie
  lives in `getAccess()`, and this getter derives from that.
- `size_t getNumberOfElements() const`, `size_t getElementBytes() const` and
  `size_t getElementPitchBits() const` (new): the geometry is
  inherited only where the entry omits its whole `address` and gives no `channels`.
  Each getter returns the entry's own value, else the inherited one, else the
  default: `1` for the number of elements, `4` for the element size, and
  `getElementBytes() * 8` for the pitch.
  - An entry with `channels` uses its own `pitch` in bytes times 8, `validate()`
    having rejected a `channels` entry without `pitch`.
  - An own `bytesPerElement` changes the element size only, not an inherited pitch.
  - One private predicate serves all three getters, each asking the parent for its
    own value.
- `std::optional<SelectedBy> getSelectedBy() const` and `RegisterPath
  getSelectedBySource() const` (new): the own selector, else the parent's, for a
  module as well. A void-typed entry never declares one, and `validate()` has
  shown that the entry declares none below an entry that declares one, so an
  inherited selector is never overridden. The source is `getPath()` where the entry
  declares the selector, else the parent's source, and is empty at the top, as the
  `addInfos` argument is today.
- `Representation`, `engineeringUnit`, `description`, `doubleBuffering` and
  `channels` are never inherited. The class's own members read them directly, so
  they need no getter.
- `void fill(NumericAddressedRegisterInfo& info) const` (modified): writes
  `info.pathName` from `getPath()` and the non-inheritable members, and takes every
  other value from the getters. It holds no inheritance logic, needs no path
  argument, and rejects nothing, `validate()` having run first.
  - It applies the register-level selector through `applyRegisterSelectedBy`,
    fed with `getSelectedBy()`, where the entry gives no `channels` and is
    no VOID. The helper now takes the selector to apply and no longer chooses
    between an own and an inherited one. Next to `channels` the selector reaches
    the channels only, which `Channel::fill` and the slices use as today;
    `getSelectedBySource()` feeds the read-only message.
  - It writes the own `representation`. `validate()` has rejected one next to
    `channels`, so `Representation::fill` never appends a channel to a 2D register.
  - A register that gives no `representation` and no `channels` gets a fixed-point
    `Representation` whose `width` is the element size in bits and whose
    `bytesPerElement` is the effective value, so `bytesPerElement: 8` yields a
    64-bit word.
  - A given `representation` that omits `width` gets the same width, the element
    size in bits. `_width` becomes `std::optional<size_t>` so that an own width is
    told from an omitted one, and the member initialiser no longer supplies `32`.
    An omitted `representation` leaves its type at `representationNotSet`; the
    `representationNotSet` recursion in `Representation::fill` goes, since `fill`
    supplies an explicit representation for every register and every channel.
  - It clears `info.channels` before filling, since the default constructor of
    `NumericAddressedRegisterInfo` already creates a channel; `addInfos` no
    longer clears it.
  - A VOID-typed entry writes `elementPitchBits` 0, `nElements` 0 and the `nodata`
    descriptor, leaves the address empty, and yields the single channel that
    `fill` emplaces today: bit offset 0, `VOID` type, width 0, fractional bits 0,
    unsigned.
- `void validate() const` (new): the one rejection step. `addInfos` calls it before
  it fills or recurses, so no getter and no `fill` guards an invalid value.
  - It rejects, for every entry:
    - an own `address` without `offset`,
    - a non-void register whose `getAddress()` is empty, with the message
      `Address not set but representation or channels given`,
    - a channel without `offset`,
    - a `channels` entry without `pitch`, or an own `pitch` without `channels`,
    - an own `representation` next to `channels`,
    - an own `selectedBy` below an entry that declares one, on a channel or on a bit
      field as well,
    - `children` next to `channels`,
    - any member other than `representation` on a bit field,
    - an own `access` next to an own `triggeredByInterrupt`,
    - a `numberOfElements`, `bytesPerElement` or `pitch` of `0` on an entry or a
      channel,
    - a void `representation` on a channel or on a bit-field child of one,
    - the read-only selector and the empty `triggeredByInterrupt` that `fill` and
      CR-020 reject today,
    - a `doubleBuffering` whose `secondaryBufferAddress` is omitted, gives no
      `offset`, or gives a `type` or `channel`, which `DoubleBufferingInfo::fill`
      rejects today. No comparison of the two channels remains.
  - It rejects a VOID entry that gives `doubleBuffering`, `channels`, an own
    `address`, an own `selectedBy`, or an own `numberOfElements` or
    `bytesPerElement`, because such an entry carries no data word, and a VOID entry
    without an own `triggeredByInterrupt`.
  - It rejects a module that gives `access` or `triggeredByInterrupt`; the
    no-word members above apply to it as well.
  - Every message names the entry by `getPath()` and is a `logic_error`.
- `void addInfos(NumericAddressedRegisterCatalogue& catalogue, const std::string&
  name, const RegisterPath& parentName)` (modified): calls `validate()`, creates a
  register for `isRegister()` entries through `fill` and recurses into the
  sub-entries.
  - It is non-`const` only to record the links: it sets `_fullPath` of the entry
    and `child._parent = this` of each sub-entry before it recurses, so `parse`
    binds the entries by `auto&`. It keeps its `name` and `parentName` arguments,
    so the existing empty-name check stays here.
  - The `addressSetByParent`, `inheritedSelectedBy` and
    `inheritedSelectedBySource` arguments are gone: the getters reach the parent
    through the link.
  - It calls `info.computeDataDescriptor()` after `fill`, as today,
    so the descriptor matches the filled channels.
  - It enters every register through `addSliceWithBufferViews`, which adds the
    register itself and creates the `BUF0`/`BUF1` views where `fill` has set
    `info.doubleBuffer`, so no second `catalogue.addRegister` is needed.
  - The `BUF1` view takes its address from `info.doubleBuffer->address`, which
    `DoubleBufferingInfo::fill` wrote from the secondary `offset`.
- Channel and bit-field slices are created for every register with `channels`,
  at its resolved address, so an address-less `channels` entry gets them too.
  - A channel slice takes the register's resolved address plus the channel's
    `offset`, its `bytesPerElement` and the register's geometry. A bit-field slice
    takes the channel's address and element size.

### JsonAddressSpaceEntry::Channel (modified) in src/JsonMapFileParser.cc (modified)

- `Channel::offset` becomes `std::optional<size_t>`, so an omitted `offset` is
  told from `0`, which stays valid. `validate()` rejects an omitted `offset`
  before anything reads or compares the optionals.
- A bit field gives only a `representation`; address, element size and pitch are
  the channel's. `ChannelChild` loses `engineeringUnit` and `description` and keeps
  the JSON object it was parsed from, so `validate()` can reject a key other than
  `representation` and `_`-prefixed comments. Its slice carries neither of the two
  members.
- A channel without a `representation` gets a fixed-point `Representation` whose
  `width` is its `bytesPerElement` times 8.

### JsonAddressSpaceEntry::Address (modified) in src/JsonMapFileParser.cc (modified)

- `std::optional<AddressType> type`, `std::optional<size_t> channel` and
  `std::optional<HexValue> offset`. `AddressType::addressTypeNotSet` goes
  with them, since no code needs the sentinel any more.
  - An omitted field deserialises to `std::nullopt`, so a field given in the map
    is told apart from an omitted one. `WITH_DEFAULT` keeps that absence.
- `static std::pair<AddressType, size_t> resolveTypeAndChannel(const
  std::optional<AddressType>& ownType, const std::optional<size_t>& ownChannel,
  const std::optional<Address>& inherited)` (new): the single copy of the
  `type`/`channel` tie, used by `getAddress()`.
  - An own `type` wins. An omitted `type` takes the inherited `type` together with
    its `channel`. With no inherited address, `type` becomes `IO` and `channel`
    becomes `0`.
  - An own `channel` always wins.
- `static Address resolve(const Address& own, const std::optional<Address>&
  inherited)` (new): resolves an own address to a fully populated `Address` whose
  `offset` is the `offset` of that own address. `inherited` is the parent's
  resolved address, so its fields are present, and `validate()` has rejected an own
  address without `offset`, so no field is read absent. `getAddress()` returns the
  result, which `Address::fill` writes into `info` as today; the assertion there
  documents that `resolve` ran.

### JsonAddressSpaceEntry::DoubleBufferingInfo (modified) in src/JsonMapFileParser.cc (modified)

- `std::optional<Address> secondaryBufferAddress` (modified): `struct
  SecondAddress` disappears and `Address` is reused, since its `type`/`channel`
  members are what tell a map that still gives them. The whole member is optional,
  so an omitted `secondaryBufferAddress` is told from a present one.
- `void fill(NumericAddressedRegisterInfo& info) const` (modified): writes the
  secondary `offset` into `info.doubleBuffer->address`; the second buffer lies on
  the same channel.
  - It rejects nothing. `validate()` rejects an omitted `secondaryBufferAddress`,
    one without `offset`, and one that gives `type` or `channel`, and the two
    channels are not compared any more.

### Format documentation (modified) in doc/jmapFormat.dox (modified)

- `jmap_access`: a register that gives no `access` inherits the parent's access.
  Such a register also inherits the parent's `triggeredByInterrupt`, unless it
  gives an `access` of its own. A register with an own `address` inherits in the same way.
  A void-typed entry inherits neither `access` nor `triggeredByInterrupt`, while
  its own interrupt reaches its descendants like any other register's.
- `doubleBuffering`: `secondaryBufferAddress` is mandatory and gives only its
  `offset`; a `type` or `channel` on it is invalid, and both buffers lie on the
  same channel. Its example and the sentence "with the same members as a regular
  `address`" drop `type`/`channel` accordingly.
- `jmap_representation`: a register that omits its whole `representation` and
  gives no `channels` gets a fixed-point word of its element size, not a 32-bit
  word, and a `representation` that omits `width` gets the same width.
- `jmap_elements`: `numberOfElements` and `bytesPerElement` are inherited from the
  parent where the entry omits its whole `address` and gives no `channels`, with
  own values overriding the inherited ones; otherwise they default to `1` and `4`.
  A value of `0` is invalid. A channel's `bytesPerElement` is its own, never
  inherited, and `4` where omitted.
- `jmap_modules`: a module is an entry for which no address resolves on the way up
  to a void entry or the top, and which gives neither `representation` nor
  `channels`.
  A module may give `children`, `selectedBy`, `engineeringUnit` and `description`,
  and `access`, `triggeredByInterrupt`, `numberOfElements`, `bytesPerElement` and
  `doubleBuffering` on it are rejected. An entry with `channels` is a
  register on the address that resolves for it, and one with `representation` or
  `channels` but no such address is rejected.
- `jmap_address_inheritance`, the `type`, `channel` and `offset` bullets of the
  `address` object: a present `address` always gives `offset`; only `type` and
  `channel` may be omitted. Both `type` and `channel` are inherited from the
  parent. If the entry gives its own `type`, the inherited `channel` is dropped and
  an omitted `channel` is `0`. Only where the parent has no address do they default
  to `IO`/`0`.
- `jmap_channel_bitfields`: a bit field gives only a `representation`; the
  channel supplies its address, element size and pitch.
- `jmap_2d`: a 2D register has `channels`, not `children`, and no register-level
  `representation`, the two being mutually exclusive. Its `numberOfElements` is
  optional and `1` where omitted, as for every register.
- `jmap_channel_selection`: a `selectedBy` is inherited, and an inherited one is
  never overridden, so neither an entry below one that declares it nor a
  void-typed entry may give its own. This replaces the "nearest ancestor takes
  precedence" and "own `selectedBy` overrides" statements of
  `jmap_module_selection_inheritance`. Neither a channel nor a bit-field child of
  one may have a void `representation`.
- The void-typed entry forbids an own `address` and takes none from its ancestry
  either, so the address chain below it starts afresh; a `doubleBuffering`,
  `channels`, an own `selectedBy` or an own geometry on it is rejected. The
  interrupt example stays as
  it is, its top-level VOID entries having no address.
- The inheritance example states that its `STATUS` child inherits
  `numberOfElements: 2` of `APP`, instead of claiming an identical outcome to the
  example above it.

### JSON schema (modified) in schemas/jmap.schema.json (modified)

- The schema states the rules for external validators; the parser stays the
  enforcement point, and no test validates the schema.
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
  It gains `"children": false` and `"representation": false`, since a 2D register
  has neither.
- The `type` and `channel` fields: the inheritance of the `address` bullets
  above.
- `width` and the `representation` description: the defaults of
  `jmap_representation` above.
- `access` and `triggeredByInterrupt`: state the inheritance.
- `selectedBy`: the inheritance and the no-override rule of
  `jmap_channel_selection` as description only, since a per-entry schema cannot see
  the ancestor; the void-typed entry's `allOf` block rejects an own `selectedBy`.
- `channel` and `channelChild`: a `void` `representation` is rejected, since both
  need a width. The channel-child definition keeps only `representation`, so
  `additionalProperties: false` rejects every other member, and its description
  states that the channel supplies the address, element size and pitch.
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
  a format version bump: its version check stays untouched, while `validate()` owns
  the individual rejections specified above.
- Calls `addInfos` for every top-level entry straight after parsing. That loop and
  the recursion in `addInfos` bind their entries by `auto&` instead of `const
  auto&`, since `addInfos` assigns the parent link of the sub-entries.

### chimeratk-map-to-jmap (modified) in tools/chimeratk-map-to-jmap (modified)

- Writes `type` and `access` on every register, because a converted map would
  resolve them differently below a `DMA`, `RO` or interrupt-bearing parent. The `INTERRUPT` guard stays, so an interrupt register
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
- Getters that throw instead of asserting: rejected. A rejection belongs in
  `validate()`, which names the entry through `getPath()`; a throwing getter would
  keep the same check in a second place.
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
loads `tests/simpleJsonFile.jmap` and applies its mutation through one shared helper
that injects it, writes a temporary file and parses it. The helper is extended from
the existing selectedBy and map-format-version ones, so no fault-only fixtures are
added. The fixture is
shared with the backend test binaries, which no case affects; a case relying on an
existing entry names it below, so a fixture edit that changes what it inherits
shows up in review. A case's `Requirement:` names the `R` bullets it covers, and
its `Necessary:` states why no other case covers them. A rejection case expects
`ChimeraTK::logic_error` and nothing more, except the one case that checks the
declaring path in the `selectedBy` message.

- `tests/executables_src/testJsonMapFileParser.cpp` (modified)
  - `TestAddressInheritanceDocumentedExample`: the documented inheritance
    example in `doc/jmapFormat.dox` - an address-bearing parent on `IO` channel 2
    at a non-zero `offset`, with one child carrying only a `representation` and
    one child whose `address` gives its own `offset`; both children take the
    parent's `type` and `channel`, the first its `offset` as well.
    - Requirement: R1 (inherited `type`/`channel`), R4 (inherited geometry).
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
    - Requirement: R1 (an own `type` drops the inherited `channel`).
    - Necessary: the example above covers the inherited `type`/`channel` pair
      only; this is the only case with a `type` of the entry's own.
  - `TestAddressInheritanceOwnChannelKept`: a child `address` giving `channel`
    and the required `offset` (its `type` omitted) below a `DMA` address on
    channel 5; the inherited `type` applies and the `channel` is the entry's
    own, `2`, not the inherited 5.
    - Requirement: R1 (an own `channel` is kept, the `type` inherited).
    - Necessary: the other cases have the `channel` omitted; only this one pins
      that an own `channel` is kept while an omitted `type` still takes the
      parent's `type`.
  - `TestChannelsEntryInheritsAddressBecomesRegister`: an entry with `channels`
    and a `pitch`, neither an own `address` nor a `representation`, below an
    address-bearing entry on `DMA` channel 3 at a non-zero offset; it yields a register on that address with one element
    and its `pitch * 8` as the pitch, its channel slices resolve to that
    address and carry the channel's `bytesPerElement`, and a bit-field `children`
    entry nested below one of the channels, say `bitShift: 8` and `width: 4`,
    resolves there too with a slice of its own. A second channel omits its
    `bytesPerElement`. It defaults to `4`, and its bit-field child is cut from that
    4-byte word; a third channel omits its `representation` and gets a fixed-point
    word of its `bytesPerElement` times 8.
    - Requirement: R5 (a 2D register and its channels).
    - Necessary: every `channels` entry in the existing maps carries its own
      `address`; the existing bit-field `children` sit below such channels, so
      an address-less register passing its address to a channel and on to a
      bit-field child is untested.
  - `TestGroupingEntryBelowAddressBecomesRegister`: an address-bearing entry on
    `DMA` channel 3 at a non-zero offset, with `bytesPerElement: 8`, `access: RO`
    and `numberOfElements: 3`, whose child gives only `children`, with an
    address-less grandchild carrying a
    `representation`; the child inherits the address and the access and yields a
    register of its own with a 64-bit fixed-point word and the inherited
    `numberOfElements` of 3, and the grandchild is a `READ_ONLY` register on the
    same address with the same `numberOfElements`.
    - Requirement: R7 (a grouping entry is a register), R3, R4.
    - Necessary: the existing maps have modules above a register-bearing entry
      (`APP`, `MY_INTC`) and a module without any address (`MY_INTC.SUB0`), but no
      grouping entry below an address with properties to inherit. One level
      suffices, the pass-through is the recursion itself.
  - `TestModuleWithPropertiesRejected`: a true module (no address in its ancestry)
    that gives `access`, a second one that gives `triggeredByInterrupt`, a third
    that gives `numberOfElements`, a fourth that gives `bytesPerElement` and a
    fifth that gives `doubleBuffering` are rejected; a sixth one that carries only
    `engineeringUnit` and `description` next to `children` parses. A module with
    `pitch` is rejected by the `pitch`-without-`channels` rule.
    - Requirement: R11 (a module rejects data members).
    - Necessary: `MY_INTC` in the fixture is a true module, and
      `TestRegisterWithoutAncestorAddressRejected` covers only the
      `representation`; the other members are rejected by their own check, and no
      fixture module carries `engineeringUnit` or `description`.
  - `TestGeometryInheritanceAndOwnSampleWidth`: an address-bearing parent with
    `numberOfElements: 2` and `bytesPerElement: 8` and three address-less children
    carrying a `representation`: the first inherits both values and the parent's
    `elementPitchBits` of 64, the second gives `bytesPerElement: 2`, keeps
    `numberOfElements` 2 and keeps the inherited `elementPitchBits` of 64, the
    third gives `numberOfElements: 3`, which supersedes the inherited 2, and a
    fourth gives `representation` with only its `type`, so its `width` defaults to
    the element size in bits.
    - Requirement: R4, R6 (inherited geometry and overrides), R7 (an omitted
      `width`).
    - Necessary: no existing case has an address-less child below a parent with a
      non-default geometry; both overrides need a case, the third child shows an
      own value winning over an inherited one, and no case gives a `representation`
      without `width`.
  - `TestChannelsWithOwnRepresentationRejected`: a register with `channels`, a
    `pitch`, `numberOfElements: 2` and `bytesPerElement: 4` that also gives a
    `representation` is rejected.
    - Requirement: R5 (`channels` and `representation` are mutually exclusive).
    - Necessary: no fixture gives both, and the parser silently ignored the
      `representation`.
  - `TestVoidChannelRejected`: a channel of a 2D register whose `representation`
    is `void` is rejected, and so is a bit-field child of a channel with one.
    - Requirement: R5 (no void channel or channel child).
    - Necessary: no fixture has a void channel, and `Channel::fill` would emplace a
      VOID channel with a width instead of rejecting it.
  - `TestTwoDimensionalEntryRestrictions`: a 2D register that also gives
    `children` is rejected, and so is a bit field that gives `bytesPerElement` or
    `engineeringUnit` next to its `representation`.
    - Requirement: R5 (a 2D register has no `children`, a bit field gives only a
      `representation`).
    - Necessary: no fixture gives either, and the parser previously ignored the
      extra members.
  - `TestChannelsKeepOwnGeometry`: a child of a parent with `numberOfElements: 2`
    giving `channels` with a `pitch` and no `numberOfElements`; it yields one
    element, so the parent's value does not reach it.
    - Requirement: R4 (no geometry inheritance with `channels`).
    - Necessary: `TestAddressInheritanceDocumentedExample` pins the positive half
      of the rule and the example's `CONTROL` child the own-`address` half; this
      case adds the `channels` half, which no other case reaches.
  - `TestAddressDefaultsWithoutAncestor`: a top-level entry whose `address` gives
    only `offset` resolves to `IO`, channel `0`, at that offset.
    - Requirement: R1 (`IO`/`0` without a parent).
    - Necessary: no case has a top-level entry without any parent, while
      `TestVoidEntryBlocksAddressInheritance` reaches the same default below a
      chain that a VOID entry ends.
  - `TestAddressWithoutOffsetRejected`: an entry below a parent with an `offset`
    that gives its own `address` without one is rejected.
    - Requirement: R1 (`offset` is never inherited).
    - Necessary: no existing map has an address without `offset`.
  - `TestSecondaryBufferAddressWithoutOffsetRejected`: a present
    `secondaryBufferAddress` without `offset`, and a `doubleBuffering` object that
    omits `secondaryBufferAddress` altogether, are rejected.
    - Requirement: R2 (`offset` is required).
    - Necessary: no existing map has a secondary address without `offset`, and
      every existing `doubleBuffering` gives `secondaryBufferAddress`, so both
      injected maps are new: one with the member present without `offset`, one
      with it omitted.
  - `TestSecondaryBufferAddressWithTypeRejected`: a double-buffered register whose
    primary `address` is `DMA` on channel 1 and whose `secondaryBufferAddress`
    gives its own `type: DMA` next to the required `offset` is rejected, and a
    second mutation giving a `channel` is rejected too.
    - Requirement: R2 (`type`/`channel` are invalid).
    - Necessary: every existing map gives a `type` there, so both mutations are
      new; without the rejection the value would silently have to match the
      primary channel.
  - `TestAccessInheritanceWithOwnAddress`: an address-bearing parent with
    `access: RO`, a `representation` of its own (`IEEE754`, `width: 64`),
    `engineeringUnit` and `description` and two
    children whose own `address` carries only `offset`; both are `READ_ONLY`. The
    first gives its own `representation` and is not reinterpreted by the parent's;
    the second omits it and gets the defaulted fixed-point word, not the parent's.
    The two children leave `engineeringUnit` and `description` empty, so these
    fields are not inherited.
    - Requirement: R3 (`access`), R9 (never inherited).
    - Necessary: no fixture combines an own `address` with an omitted `access`
      below a parent whose access is not the default.
  - `TestTriggeredByInterruptInheritance`: an address-bearing parent with
    `triggeredByInterrupt` and five children; the address-less one without
    `access` is an `INTERRUPT` register with the parent's interrupt id, the
    address-less one giving `access: RO` is `READ_ONLY` without an interrupt id,
    the one whose own `address` gives only `offset` is `INTERRUPT` as well, a
    void-typed child with its own `triggeredByInterrupt` keeps that id instead of
    the parent's, and a child below a parent with `access: RO` that gives its own
    `triggeredByInterrupt` is `INTERRUPT` rather than `READ_ONLY`.
    - Requirement: R3 (`triggeredByInterrupt`).
    - Necessary: no fixture reaches this rule, since no interrupt register in the
      fixture has address-less children; `DAQ.CTRL` and `DAQ.FD` carry
      `triggeredByInterrupt` but stand alone, and the interrupt registers that use
      `channels` have bit-field children in a reduced struct that never reaches
      `fill`.
  - `TestTriggeredByInterruptInheritsName`: an address-bearing parent whose
    `triggeredByInterrupt` names another interrupt register of the fixture, and an
    address-less child with a `representation`; both resolve to that register's id
    after `finaliseInterruptBookkeeping()`.
    - Requirement: R3 (`triggeredByInterrupt`).
    - Necessary: CR-020 lets the field name a register; the declaration is a
      variant and nothing covers a name inherited from a parent.
  - `TestRegisterWithoutAncestorAddressRejected`: an entry with `channels` and a
    `pitch`, and an entry with a `representation`, each with no address anywhere
    in its ancestry, are rejected. The `pitch` is given so that the `channels` case
    is not caught by the `channels`-without-`pitch` rule.
    - Requirement: R1 (no address on the way up).
    - Necessary: no existing map has a register without an address in its
      ancestry. Both entries reach the same rejection, but they cover the two
      members that make an entry a register without an address.
  - `TestChannelsEntryWithoutPitchRejected`: a `channels` entry without `pitch`
    is rejected, now that an address-less `channels` entry parses.
    - Requirement: R5 (`channels` requires `pitch`).
    - Necessary: the schema requires `pitch` next to `channels`, the parser does
      not, and an omitted `pitch` would otherwise be accepted silently.
  - `TestChannelWithoutOffsetRejected`: a channel of a 2D register with its
    `offset` removed is rejected.
    - Requirement: R5 (a channel gives its `offset`).
    - Necessary: `Channel::offset` is uninitialised today, so no fixture reaches
      the rejection, and the schema's requirement is unenforced by the parser.
  - `TestZeroGeometryRejected`: a `numberOfElements: 0` and a `bytesPerElement: 0`
    are rejected on an otherwise valid register, a channel's `bytesPerElement: 0`
    on a 2D register, and a `pitch: 0` on that 2D register.
    - Requirement: R4 (`0` is no geometry value).
    - Necessary: `0` means "not set" today, so the new reading needs its own case.
  - `TestPitchWithoutChannelsRejected`: an own `pitch` on a register that gives no
    `channels` is rejected, the counterpart of
    `TestChannelsEntryWithoutPitchRejected`.
    - Requirement: R5 (`pitch` exactly where `channels` are).
    - Necessary: the schema forbids the combination and the parser ignores it
      today.
  - `TestDoubleBufferingWithInheritedAddress`: a `doubleBuffering` register
    without an own `address`, below an address-bearing entry on `DMA` channel 3 at
    a non-zero offset, gets its `BUF0`
    view on the resolved address and its `BUF1` view on the address from
    `secondaryBufferAddress`, both on the same channel.
    - Requirement: R2, R1 (a `doubleBuffering` register below an address).
    - Necessary: every existing `doubleBuffering` gives its own `address`, so an
      inherited one with its views is untested.
  - `TestSelectedByReadOnlyMessage`: a writable register whose `selectedBy` is
    declared on a parent is rejected, and the message names the parent's path.
    - Requirement: R8 (`selectedBy` is inherited).
    - Necessary: `getSelectedBySource()` replaces the `addInfos` argument, and no
      case checks the declaring path in the message.
  - `TestSelectedByOverrideRejected`: a child below a `selectedBy`-declaring
    parent that gives its own `selectedBy` is rejected, and so is a void-typed
    entry with one.
    - Requirement: R8 (`selectedBy` is inherited and never overridden), R10.
    - Necessary: the fixture's `selectedBy` cases declare it on one entry only, so
      an own value below a declarer parses today.
  - `TestAccessAndInterruptRejected`: an entry giving both `access` and
    `triggeredByInterrupt` is rejected.
    - Requirement: R3 (mutually exclusive).
    - Necessary: the check moves from the entry branch into `addInfos`, and no
      existing case reaches it.
  - `TestVoidEntryRejections`: a VOID-typed entry with a `doubleBuffering`, a
    second one with `channels`, a third one with an own `address`, a fourth one with
    an own `numberOfElements`, a fifth one with an own `bytesPerElement`, a sixth
    one with an own `pitch` and a seventh one without `triggeredByInterrupt` are
    rejected.
    - Requirement: R10 (what a void-typed entry rejects).
    - Necessary: the existing VOID entries carry none of the seven and all sit
      below an address-bearing entry. The missing `triggeredByInterrupt` is
      rejected by `fill` today and moves into `addInfos` with the other six. An
      own `selectedBy` is covered by `TestSelectedByOverrideRejected`.
  - `TestVoidEntryBlocksAddressInheritance`: a VOID-typed entry below an
    address-bearing parent is a register with `address` `0` and with `nElements`
    0, while that parent sits at a non-zero offset on `DMA` channel 3. A child
    injected
    below the VOID entry that gives a `representation` but no own `address` is
    rejected. A sibling child whose `address` gives only `offset` and which gives
    no `access` parses as an `INTERRUPT` register with the VOID entry's id,
    resolved to `IO` and channel `0`, so it took the `IO` default rather than a
    value from above. That
    child also carries a `selectedBy` declared above the VOID entry, which passes
    through. The top-level VOID entries of the interrupt example in
    `doc/jmapFormat.dox` parse as well.
    - Requirement: R10 (the void exception), R8 (`selectedBy` passes through).
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
  `secondaryBufferAddress`; `tests/selectedByInheritance.jmap` loses the own
  `selectedBy` of `INHERIT.REG_B` and `INHERIT.REG2D_OWN`.
- Not repeated, already covered: `selectedBy` inheritance from a module stays
  covered by `TestSelectedByInheritance`, whose assertions on a child's own
  selector go with the two fixture entries that `TestSelectedByOverrideRejected`
  now rejects. `chimeratk-device-to-jmap` is unchanged and keeps its round trips;
  nothing runs `chimeratk-map-to-jmap`, which has no test harness (DI-1).

## Deferred issue

- DI-1 [ACCEPTED] `chimeratk-map-to-jmap` has no test harness, so nothing
  verifies the maps it emits under the rules of this change; its output shape is
  checked by review only.
