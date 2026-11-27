# CR-018: per-field inheritance of JMAP addresses

Synopsis: The parser resolves the values an entry takes from its ancestors ad hoc,
and differently per value. The change gives each value one rule, resolves it in a
`const` getter, rejects the invalid cases in a single `validate()` step, and states
the result in the format documentation and the schema.

Status: PLANNED

## Requirements

Aspect: inheritance. An entry is an object in `addressSpace` or in a `children`
object; its parent is the entry whose `children` hold it, and a top-level entry has
none. A value an entry gives itself is its own; every other value it inherits from
its parent. Inheriting repeats up the chain, so a parent that gives no value of its
own passes on the one it inherited.

An entry resolves an `offset` where the search up its `address` chain finds one,
stopping at a void-typed entry or a top-level entry. The chain is the entry's
ancestors that give or inherit an `address`. A void-typed entry gives a
`representation` whose `type` is `void`, so it resolves no `offset` and passes none
on. A grouping entry gives neither `representation` nor `channels` and still
resolves an `offset`, so it becomes a register carrying only `children`; a module is
such an entry without a resolved `offset`.

- R1: `address`
  - An omitted `address` is inherited as a whole, `offset` included.
  - An own `address` always gives `offset`. Only `type` and `channel` may be omitted
    in it, and they are inherited.
  - An own `type` drops the inherited `channel`; an omitted `channel` is then `0`,
    since a channel means something different per `type`.
  - An own `channel` is kept, whatever the `type` resolves to.
  - Where no entry above gives an address, `type` is `IO` and `channel` is `0`.
  - An entry that gives a non-void `representation` or `channels` must resolve an
    address.
- R2: `doubleBuffering`
  - It requires a resolved address, since the two buffers lie at two addresses. An
    entry that gives it and resolves none is a module, where R11 rejects it.
  - `secondaryBufferAddress` is mandatory and gives only `offset`, which is required.
  - A `type` or `channel` on it is rejected. The second buffer lies on the same
    channel as the primary, and the parser derives that from the primary address.
- R3: `access` and `triggeredByInterrupt`
  - The pair is inherited together: an entry that gives neither takes the parent's
    `access`, and with it the parent's `triggeredByInterrupt` and the `INTERRUPT`
    access that implies.
  - An own `triggeredByInterrupt` gives the entry `INTERRUPT`, whether or not an
    ancestor declares one.
  - An own `access` suppresses the inheritance: the entry keeps its own access and
    inherits no `triggeredByInterrupt`, so no interrupt id reaches it.
  - An entry must not give both `access` and `triggeredByInterrupt`.
- R4: the geometry, `numberOfElements` and `bytesPerElement`
  - Both are inherited only where the entry omits its whole `address` and gives no
    `channels`; otherwise the entry's own values apply, and `1` and `4` where it omits
    them.
  - `0` is rejected for `numberOfElements`, `bytesPerElement` and `pitch`, on an entry
    as well as on a channel.
- R5: 2D registers and their channels
  - A register-level `representation` and `channels` are mutually exclusive, so
    either next to the other is rejected.
  - `pitch` is given exactly where `channels` are, one rule covering both directions:
    `channels` without `pitch` is rejected, and so is `pitch` without `channels`.
  - A 2D register has no `children`; `children` next to `channels` is rejected.
  - Each channel gives its own `offset`, `0` included; a channel without `offset` is
    rejected. Its `bytesPerElement` is optional, never inherited, and `4` where
    omitted.
  - A channel without a `representation` gets the fixed-point word of R7, at its own
    element size.
  - The `children` of a channel are bit fields of its element. A bit field gives a
    `representation`, and `engineeringUnit` or `description` where it wants them; its
    address and element size are the channel's. A further member is not the parser's
    check: the schema rejects it.
  - A bit field may omit its `bitShift`, which is then `0`, and its `width`, which
    is then the element width. A child that spans the whole element or whose
    `bitShift + width` exceeds it is ignored, with no slice for it, as CR-003
    requires for forward compatibility.
  - A void `representation` on a channel or a bit field is rejected.
- R6: a `pitch` is never inherited: only an entry with `channels` gives one, and such
  an entry has no `children`. An own `bytesPerElement` changes the element size only.
- R7: a register that gives neither `representation` nor `channels` gets a fixed-point
  word at its resolved address whose width is its element size in bits; a
  `representation` that omits `width` takes that width as well. A `string`
  `representation` is the exception: its width is always `8`, whatever the element
  size, since a character is one byte and the map states no character width in bits.
  An own `width` other than `8` is rejected. Every entry that
  gives neither `representation` nor `channels` and resolves an `offset` is such a
  register, a grouping entry among them, and the parser emits it in addition to its
  children.
- R8: `selectedBy`
  - `selectedBy` is inherited.
  - An inherited `selectedBy` is never overridden: neither an entry nor a channel
    below one that declares `selectedBy` may give its own. A selector
    selects a whole subtree: a selected 2D register already selects each of its
    channels, and a selected parent already selects its children, so a selector of its
    own below a declaring entry would contradict the selection above it.
  - A channel with no declaration above it may give its own selector, which its bit
    fields take. A bit field never declares one, and neither does a module, which
    gives only `children` (R11).
  - A selector states when the data is valid to read, so a register that resolves one
    must resolve `access` `READ_ONLY` or `INTERRUPT`; a writable register carrying a
    selector, own or inherited, is rejected.
  - A `selectedBy` on a bit field is not the parser's check, which ignores the key:
    the schema rejects it, as R5 says for any further member on a bit field.
- R9: `representation`, `engineeringUnit`, `description`, `doubleBuffering` and
  `channels` are never inherited, so a child takes the parser's default where the
  member is mandatory. `channels` can never reach a child anyway, since R5 rejects
  `children` next to `channels`; the other four a child gives itself or leaves empty.
- R10: a void-typed entry
  - It holds no data, so `channels`, an own `address`, `numberOfElements`,
    `bytesPerElement`, `pitch` and `doubleBuffering` are rejected.
  - It must give its own `triggeredByInterrupt`, because it inherits neither `access`
    nor an interrupt id; its access is therefore `INTERRUPT`.
  - `engineeringUnit` and `description` stay allowed on it, although R11 rejects them
    on a module: it describes the interrupt source it represents, while a module
    describes nothing.
  - It has no `selectedBy` of its own, while an inherited one passes through it.
  - It resolves no address, whatever its ancestors give, and passes on neither
    `numberOfElements` nor `bytesPerElement`, so its children are in the position of
    top-level entries.
  - Its own `triggeredByInterrupt` reaches its descendants like the value of any
    other register.
- R11: a module
  - It resolves no address on the way up to a void-typed entry or the top, and gives
    neither `representation` nor `channels`.
  - It groups only: every member but `children` is rejected, `access`,
    `triggeredByInterrupt`, `numberOfElements`, `bytesPerElement`,
    `doubleBuffering`, `engineeringUnit`, `description` and `selectedBy` among them.
    Where a value should reach a subtree, a register above declares it.

Aspect: compatibility. Every map that declares the format version obeys the rules
below, and the maps whose behaviour changes are listed and accepted.

- R12: The format version stays `1.0`. A map that violates the rules stops parsing,
  among them one that
  - gives an own `address` without `offset`, or gives a channel without `offset`,
  - gives an own `selectedBy` below an entry that declares one, on that entry or on
    a channel below it,
  - resolves a `selectedBy` on a writable register, own or inherited,
  - gives `type` or `channel` on a `secondaryBufferAddress`,
  - gives `channels` and a register-level `representation` together,
  - gives `children` next to `channels`,
  - gives any member other than `children` on a module,
  - gives `pitch` without `channels`, or `channels` without `pitch`,
  - gives `numberOfElements`, `bytesPerElement` or `pitch` as `0`,
  - gives a `width` other than `8` on a `string` `representation`,
  - gives a void `representation` on a channel or on a bit field,
  - gives `access` and `triggeredByInterrupt` together,
  - gives a void-typed entry an own `address`, `channels`, `numberOfElements`,
    `bytesPerElement`, `doubleBuffering` or `selectedBy`, or omits its
    `triggeredByInterrupt`.
- R13: The following maps behave differently after the change. These differences are
  accepted, so no migration follows.
  - They resolve differently:
    - a register with its own `address` and no `access` below a parent with a
      non-`READ_WRITE` access takes the inherited access and interrupt,
    - an own `address` with an omitted `type` below a parent on another channel takes
      the parent's `type`, so the register moves to another `type`/`channel`
      combination,
    - an address-less entry below a parent that gives a geometry takes the parent's
      `numberOfElements` and `bytesPerElement`, and with them its element size and the
      pitch that follows from it; a `pitch` itself is never inherited,
    - a register without a `representation` gets a word of its effective width, not
      32 bits, and a given `representation` that omits `width` gets its element size
      in bits instead of 32,
    - a channel without a `representation` gets a word of its own element size
      instead of a 32-bit word, and so does a channel's `representation` that omits
      `width`,
    - a void-typed entry writes no address and `elementPitchBits` 0, where it took the
      parent register's address and pitch from the catalogue,
    - a double-buffered register that inherits its address now gets its `BUF0` and
      `BUF1` views, which the base created only for an entry with an own `address`,
    - an address-less entry with a `representation` below a bit-range register is no
      longer a bit range itself, since the base copied the parent's `isBitRange`,
  - They are rejected today and parse after the change:
    - an address-less entry with `channels`, which gets its slices,
    - an entry that gives only `children` below an entry that resolves an `offset`,
      which becomes a grouping register,
    - a `secondaryBufferAddress` that gives neither `type` nor `channel`,
  - They stop parsing as R12 lists:
    - a `string` `representation` that gives a `width` other than `8`, while one that
      omits its `width` gets `8` instead of 32 bits.

Aspect: documentation and tools. The format documentation and the schema state the
rules above, and the map generator emits maps that satisfy them.

- R14: `doc/jmapFormat.dox` and `schemas/jmap.schema.json` state every rule above,
  each rule in the section of the member it concerns.
- R15: `chimeratk-map-to-jmap` emits maps that satisfy the rules above:
  - an `offset` in every `address`, an explicit `type` on every address it writes,
    the `IO` ones included and not only the `DMA` ones, and an explicit `access` on
    every register except an interrupt register,
  - no zero geometry: a member it cannot give a value for is omitted, not written as
    `0`,
  - an explicit `representation` width wherever omitting it would change the
    resolved width, and none on a `string`, whose width the parser fixes at `8`,

## Specifications

The parse runs in three steps, kept apart. The JSON deserialises into the entry
objects, `validate()` then rejects every invalid per-entry case in one place, and
only then `fill()` copies the data into the register info, taking each value from a
getter. The getters resolve inheritance and never reject: they assume valid entries,
which `validate()` has established, and `addInfos` has linked their parents
beforehand. `JsonMapFileParser::Imp::parse` keeps its checks across registers after
the parse, since a per-entry validator cannot make them. The parser rejects a defined
key in an invalid place and ignores a key that no definition lists; the schema
forbids both.

A slice is a read-only register the parser creates for one channel of a 2D register,
or for one bit field of such a channel, and which the catalogue exposes beside the
2D register itself. Its access is `READ_ONLY`, or `INTERRUPT` for a slice of an
interrupt-driven register; both are read-only, and `INTERRUPT` adds
`wait_for_new_data`.

### JsonAddressSpaceEntry (modified)

Location: src/JsonMapFileParser.cc (modified)

The bullets follow the parse: the data members, the getters that resolve the values,
then `validate()`, `fill()` and `addInfos()`.

- `const JsonAddressSpaceEntry* _parent` (new): the direct parent entry, and
  `nullptr` at the top level. `RegisterPath _fullPath` (new) is the entry's fully
  qualified path, which `getPath()` returns.
  - The recursion in `addInfos` assigns `_parent` on every child where it recurses,
    so a getter always finds a complete chain.
  - The pointer refers into the tree of entries. A parent owns its sub-entries
    through `std::map` nodes, so it outlives its child, and no pointer leaves the
    tree: `addressSpace` is a local of the parse function and only copies of
    `NumericAddressedRegisterInfo` leave it.
  - The reader builds every entry in its `std::map` node before the recursion starts,
    and nothing copies or moves a linked entry afterwards, so no copy can carry a
    stale `_parent`.
- Data members of `JsonAddressSpaceEntry` itself become private and keep their JSON
  key names, so `NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT` maps them as today;
  each of them carries a `// NOLINT(readability-identifier-naming)` comment, since
  the macro derives the key from the member name and the project's naming rule would
  otherwise reject the member. Every getter starts with `get`, so a member and its
  getter cannot share a name. The nested JSON shapes (`Address`,
  `Representation`, `Channel`, `ChannelChild`, `DoubleBufferingInfo`) are not reached
  by a getter and keep their names and their macro reader. All inheritance is
  resolved by `const` getters that consult `_parent`. The only non-`const` steps
  are the recursion in `addInfos` recording `_fullPath` and each child's parent
  link, and each rule is stated in one getter only.
  - `numberOfElements`, `bytesPerElement` and `pitch` become `std::optional<size_t>`,
    so an own value is told from an omitted one and a module can reject them.
  - `triggeredByInterrupt` keeps CR-020's optional `InterruptId`, so the getters
    pass a register name and a numeric array on alike.
  - `representation` becomes `std::optional<Representation>`: the map's absence is
    `std::nullopt`, while a void `representation` stays a present value. The nested
    type keeps `FIXED_POINT` as its default `type`, as today.
  - `access` becomes `std::optional<NumericAddressedRegisterInfo::Access>`, so an
    optional carries the absence; the parser's `Access` enum with its `accessNotSet`
    goes, since its value collided with the catalogue's `INTERRUPT`.
  - `access` and `representation.type` use the catalogue's
    `NumericAddressedRegisterInfo::Access` and `::Type`; the parser's `Access` and
    `RepresentationType` enums go, and with them the `int(...)` mirroring of one enum
    onto the other. `access` needs the swap because its sentinel collided with the
    catalogue's `INTERRUPT`; `representation.type` follows, since the two enums have
    the same enumerators and one enum per value avoids a second mapping. The JSON
    strings map through `NLOHMANN_JSON_SERIALIZE_ENUM`
    declared for the catalogue enums in `namespace ChimeraTK` in
    `src/JsonMapFileParser.cc`, because the serialiser finds it by argument-dependent
    lookup there; the overloads exist in that translation unit only, and an unknown
    string, `INTERRUPT` among them, falls back to the first listed value, which is
    `READ_ONLY`, as today. `AddressType`
    (`IO`/`DMA`) stays parser-local, having no catalogue counterpart.
  - `_parent` and `_fullPath` are not JSON keys, so they are absent from the macro's
    key list and are neither read from nor written to JSON.
  - A value the map does not give is `std::nullopt`; `0` never stands for an absent
    value.
- `const RegisterPath& getPath() const` (new): `_fullPath`, the fully qualified path
  of the entry. `addInfos` records it from its `name` and
  `parentName` arguments, with the dotted alt separator as today, before it calls
  `validate()`, so every rejection message names the entry.
  - It replaces the `name` and `parentName` arguments of `fill`, and is what the
    error messages and `info.pathName` use.
  - It needs no second lookup to name the entry that declared the effective
    selector.
- `std::optional<Address> address` in `JsonAddressSpaceEntry`, so an omitted
  `address` is a `std::nullopt` and an `address: {}` is a present value whose fields
  are all omitted.
- `bool isRegister() const` (new): true where `representation` is present (a void one
  included), where the entry gives a non-empty `channels` object, or where it
  resolves an `offset`. An empty `channels` counts as omitted, as today. An entry
  with none of the three is a module.
- `bool isVoid() const` (new): true where a present `representation` has the `type`
  `void`. Every getter of an inherited value, `fill()` and `validate()` ask it, so the
  void rule lives here and not once per getter.
- `std::optional<HexValue> getOffset() const` (new): the own `offset`, else the
  parent's, so the search continues up the chain; `std::nullopt` at the top.
  - A void-typed entry returns `std::nullopt` whatever the parent gives, so it is
    the one register without a resolved address, writes none into the register info
    and passes none on: the search of a child stops at it.
- `std::pair<AddressType, size_t> getTypeAndChannel() const` (new): the resolved
  `type` and `channel`, formed on each call from the entry and its parent, never
  stored.
  - An own `address` starts from the parent's `type` and `channel`, or `IO`/`0` where
    the parent has no address, and an own `type` replaces them by its `type` and
    channel `0`.
  - An own `channel` replaces the inherited one.
  - Without an own `address` the entry returns the parent's `type` and `channel`.
  - A void-typed entry returns `IO`/`0` whatever the parent gives, so a child that
    brings its own `address` starts from `IO` and `0`.
- `Representation effectiveRepresentation(const std::optional<Representation>&
  given, size_t elementBits) const` (new): the given representation with its omitted
  `width` set to `elementBits`, or `8` for a `string`; where the entry gives none, a
  fixed-point representation of `elementBits`. `fill` and the slice creation both
  use it, so the width default of R7 exists once and not per call site.
- `NumericAddressedRegisterInfo::Access getAccess() const` (new): the own `access`.
  Otherwise `INTERRUPT` where the entry or an ancestor declares a
  `triggeredByInterrupt` that the entry does not suppress. Otherwise the parent's
  value, and `READ_WRITE` at the top. The return type is the catalogue's enum because
  it has an `INTERRUPT` value, which the member shares, so the member's value needs
  no conversion.
  - A void-typed entry inherits neither and takes its own `triggeredByInterrupt`,
    which gives it `INTERRUPT`.
- `std::optional<NumericAddressedRegisterInfo::InterruptId>
  getTriggeredByInterrupt() const` (new): the own declaration, else the parent's
  resolved value, so the chain passes a declaration through an entry that inherited
  it. Empty where the entry gives an `access` of its own, which suppresses the
  inheritance. `fill` writes this value into `info.interruptId`, which CR-020
  resolves after the parse.
  - The pair rule of R3 is not repeated here: `validate()` rejects an own `access`
    next to an own `triggeredByInterrupt`, so an entry that reaches this getter
    either gives no `access` or no `triggeredByInterrupt`.
- `size_t getNumberOfElements() const`, `size_t getElementBytes() const` and
  `size_t getElementPitchBits() const` (new): `numberOfElements` and
  `bytesPerElement` are inherited only where the entry omits its whole `address` and
  gives no `channels`. Each getter returns the entry's own value, else the inherited
  one, else the default: `1` for the number of elements and `4` for the element size.
  The pitch is `getElementBytes() * 8`, or the own `pitch` in bytes times 8 for an
  entry with `channels`; it is never inherited, since only a `channels` entry gives
  one and such an entry has no `children`.
  - An own `bytesPerElement` changes the element size only.
  - One private predicate serves all three getters, each asking the parent's getter,
    so a value repeats through an ancestor that only inherited it.
  - A void-typed entry passes neither `numberOfElements` nor `bytesPerElement` on, as
    it passes no address, so the geometry getters treat it like the top and return the
    defaults below it.
- `std::optional<SelectedBy> getSelectedBy() const` and `RegisterPath
  getSelectedBySource() const` (new): the own selector, else the parent's, also
  where the entry is a module, which passes an inherited one on. A void-typed entry
  never declares one, and `validate()` has shown that the entry declares none below
  an entry that declares one, so an inherited selector is never overridden.
  - The comment on the `selectedBy` member that calls a register-level selector next
    to `channels` an authoring error goes: the register-level selector is now the
    default for every channel that has none above it.
  - `getSelectedBySource()` is `getPath()` where the entry declares the selector,
    else the parent's source, and is empty at the top, as the `addInfos` argument is
    today. It feeds the message of the read-only-selector rejection.
- `Representation`, `engineeringUnit`, `description`, `doubleBuffering` and
  `channels` are never inherited. The class's own members read them directly, so
  they need no getter.
- `void fill(NumericAddressedRegisterInfo& info) const` (modified): writes
  `info.pathName` from `getPath()` and the non-inheritable members, and takes every
  other value from the getters. It holds no inheritance logic, needs no path
  argument, and rejects nothing, `validate()` having run first.
  - It writes `info.address` from `getOffset()` and `info.bar` from
    `getTypeAndChannel()` as `channel + (type == DMA ? 13 : 0)`, the encoding
    `Address::fill` applied until now; a void-typed entry leaves both at their
    defaults.
  - It creates `info.doubleBuffer` where the entry gives a `doubleBuffering`, and
    resets it otherwise, before `DoubleBufferingInfo::fill` writes into it.
  - It writes the register's selector into the channel it created from
    `getSelectedBy()`, where the entry gives no `channels` and is no VOID. The base's
    `applyRegisterSelectedBy` helper, which the base called from the branch of an own
    `address` and from the `addressSetByParent` branch and which chose between an own
    and an inherited selector, is inlined here: the merged `fill` has one path and
    the choice has gone. Next to `channels`, `fill` hands `Channel::fill` and the
    channel slices the selector the channel carries, its own where it gives one, else
    the one the entry resolved; `validate()` has shown that the two cannot both
    apply. It converts the JSON `SelectedBy` to the catalogue's `SelectedBy`, with
    its dotted alt separator, through one file-local helper that `Channel::fill` and
    the slice block use as well.
  - Where the entry gives no `channels`, it creates the register's own channel from
    `effectiveRepresentation` above, which supplies the fixed-point word of an entry
    that gives none and the `width` an own `representation` omits. A `channels` entry
    creates its channels through `Channel::fill`, to which `fill` hands the resolved
    representation as well.
  - It marks `info.isBitRange` where a channel's `representation` gives a non-`0`
    `bitShift`, which `Channel::fill` did until now.
  - It clears `info.channels` before filling, since the default constructor of
    `NumericAddressedRegisterInfo` already creates a channel; `addInfos` no longer
    clears it.
  - A VOID-typed entry writes `elementPitchBits` 0, `nElements` 0 and the `nodata`
    descriptor, leaves the address empty, and yields the single channel that `fill`
    emplaces today: bit offset 0, `VOID` type, width 0, fractional bits 0, unsigned.
- `void validate() const` (new): the one rejection step for the rules of the
  parser's own members. `addInfos` calls it after recording the links and the path
  and before it fills or recurses, so no getter and no `fill` guards an invalid
  value.
  - It rejects, for every entry:
    - an own `address` without `offset`,
    - a non-void register whose `getOffset()` is empty, with the message
      `Address not set but representation or channels given`,
    - a `channels` entry without `pitch`, or an own `pitch` without `channels`,
    - an own `representation` next to `channels`,
    - an own `selectedBy` below an entry that declares one, for the entry itself,
    - `children` next to `channels`,
    - an own `access` next to an own `triggeredByInterrupt`,
    - an own `width` other than `8` on a `string` `representation`,
    - a `numberOfElements`, `bytesPerElement` or `pitch` of `0` on an entry,
    - what `Channel::validate` rejects for each channel and its bit fields,
      which `validate()` calls once per channel with the entry's `getAccess()`,
      because only the channel knows its element width, its `children` and the
      selector it carries,
    - a resolved `selectedBy` on an entry whose `getAccess()` is neither `READ_ONLY`
      nor `INTERRUPT`, for its own selector and for an inherited one alike, with the
      message naming the declaring entry through `getSelectedBySource()`,
    - a `doubleBuffering` whose `secondaryBufferAddress` is omitted, gives no
      `offset`, or gives a `type` or `channel`, which `DoubleBufferingInfo::fill`
      rejects today. No comparison of the two channels remains.
  - It rejects on a void-typed entry and on a module what R10 and R11 exclude,
    including a void-typed entry without an own `triggeredByInterrupt`.
  - The empty `triggeredByInterrupt` stays rejected by
    `NumericAddressedRegisterCatalogue::finaliseInterruptBookkeeping()`, which every
    catalogue builder uses, so the rule is not repeated here.
  - Every message names the entry by `getPath()` and is a `logic_error`.
- `void addInfos(NumericAddressedRegisterCatalogue& catalogue, const std::string&
  name, const RegisterPath& parentName)` (modified): records `_fullPath` and the
  parent links, calls `validate()`, creates a register for `isRegister()` entries
  through `fill` and recurses into the sub-entries.
  - It is non-`const` only for those two recordings. It keeps its `name` and
    `parentName` arguments: a top-level entry has no `_parent` to ask for its path,
    and the empty-name check uses `name`.
  - The `addressSetByParent`, `inheritedSelectedBy` and
    `inheritedSelectedBySource` arguments are gone: the getters reach the parent
    through the link.
  - It calls `info.computeDataDescriptor()` after `fill`, as today, so the
    descriptor matches the filled channels.
  - It enters every register through `addSliceWithBufferViews`, which adds the
    register itself and creates the `BUF0`/`BUF1` views where `fill` has set
    `info.doubleBuffer`, so no second `catalogue.addRegister` is needed.
  - The `BUF1` view takes its address from `info.doubleBuffer->address`, which
    `DoubleBufferingInfo::fill` wrote from the secondary `offset`.
- Channel and bit-field slices are created for every register with `channels`, at
  its resolved address, so an address-less `channels` entry gets them too.
  - Their construction stays as today, from the filled register info: the access
    (`INTERRUPT` for an interrupt-driven register, `READ_ONLY` otherwise), the
    `bar`, the `interruptId`, the geometry, the inherited `doubleBuffer` with its
    address shifted by the channel `offset`, `isBitRange` where the channel's or the
    bit field's `bitShift` is not `0`, and the collision check that rejects a slice
    path which already exists.
  - New are only the inputs: the register's resolved address plus the channel's
    `offset`, the channel's resolved `bytesPerElement` and representation, and the
    selector.
  - Both take the width default from `effectiveRepresentation` and pass its result
    to `makeChannelInfo`, so the word default of R5 and R7 exists once and not
    separately for the register and the channel.

### JsonAddressSpaceEntry::Channel (modified)

Location: src/JsonMapFileParser.cc (modified)

- `Channel::offset` becomes `std::optional<size_t>`, so an omitted `offset` is told
  from `0`, which stays valid. `validate()` rejects an omitted `offset` before
  anything reads or compares the optionals.
  - `Channel::fill` dereferences it, `channelsInOffsetOrder()` sorts by its value,
    and the macro reader keeps reading the key `offset` into the optional, where an
    absent key leaves it `std::nullopt`.
- `Channel::selectedBy` stays: a channel below no declaring entry may give its own
  selector, and `validate()` rejects only an override. The selector a channel
  carries is its own where it gives one, else the one the entry resolved, and
  `Channel::fill` takes it from `fill`; the declaring entry is no longer a
  parameter, since the read-only rejection that named it moved into `validate()`.
  The comment that calls a channel selector an override goes with it.
- A channel without a `representation` gets a fixed-point `Representation` whose
  `width` is its element size in bits. `Channel::fill` receives it from `fill`,
  which applies `effectiveRepresentation` to the channel's own `representation` and
  the channel's element size, so the word default of R5 and R7 exists once.
- `void validate(const RegisterPath& path, const RegisterPath& selectedBySource,
  NumericAddressedRegisterInfo::Access access) const` (new): rejects, for this
  channel, an omitted `offset`, a `bytesPerElement` of `0`, a void
  `representation`, and an own `selectedBy` where `selectedBySource` is not empty,
  which is exactly the case of a declaration above; and for each of its bit fields
  an omitted `representation`. It also rejects a channel that carries a selector,
  own or inherited, where `access` is neither `READ_ONLY` nor `INTERRUPT`, with the
  message naming the declaring entry.
  - The channel is the only place that knows an element width and its own
    `children`, so `JsonAddressSpaceEntry::validate()` calls this once per channel
    instead of identifying bit fields by their position in the entry tree.
- A bit field gives a `representation`, and an own `engineeringUnit` or
  `description` where it wants them; its address, element size and pitch are the
  channel's.
  - `ChannelChild::representation` becomes `std::optional<Representation>`, so an
    omitted one is told from a given one; `Channel::validate` rejects `std::nullopt`,
    and the schema requires the key. `ChannelChild` keeps `engineeringUnit` and
    `description`, as today. The parser ignores any further member on it, and the
    schema's `additionalProperties: false` rejects it and admits the `_`-prefixed
    comments.
  - Its slice is a copy of the channel slice, with `engineeringUnit` and `description`
    taken from the bit field itself, which inherits neither (R9).
  - The skip of a bit field that spans the whole element or exceeds it stays, as
    CR-003 requires: no slice is created for it and the parse continues.

### JsonAddressSpaceEntry::Representation (modified)

Location: src/JsonMapFileParser.cc (modified)

- `width` becomes `std::optional<size_t>`, so an own width is told from an omitted
  one; the member initialiser no longer supplies `32`.
- `Representation::fill` goes: `effectiveRepresentation` supplies the width, `fill`
  creates the register's own channel, and each channel's `fill` and `makeChannelInfo`
  take the resolved representation from `fill`, so one path serves the register and
  its channels.
- A `string` type has width `8`, and `validate()` rejects an own `width` other than
  `8`.
- The nested type keeps `WITH_DEFAULT`, which reads its members by their key names,
  so `width` keeps its name and needs no `NOLINT` comment, unlike the entry class's
  members.

### JsonAddressSpaceEntry::Address (modified)

Location: src/JsonMapFileParser.cc (modified)

- `std::optional<AddressType> type`, `std::optional<size_t> channel` and
  `std::optional<HexValue> offset`. `AddressType::addressTypeNotSet` goes with them,
  since no code needs the sentinel any more; every former comparison against it
  becomes a check on the optional.
  - An omitted field deserialises to `std::nullopt`, so a field given in the map is
    told apart from an omitted one. `WITH_DEFAULT` keeps that absence.
- `Address` moves above `DoubleBufferingInfo`, which holds a
  `std::optional<Address>` and therefore needs the complete type.
- It keeps its members only: `Address::fill` goes, since `getOffset()` and
  `getTypeAndChannel()` resolve the fields and `fill` writes the result.

### JsonAddressSpaceEntry::DoubleBufferingInfo (modified)

Location: src/JsonMapFileParser.cc (modified)

- `std::optional<Address> secondaryBufferAddress` (modified): `struct SecondAddress`
  disappears and `Address` is reused, since its `type`/`channel` members are what
  tell a map that still gives them. The whole member is optional, so an omitted
  `secondaryBufferAddress` is told from a present one.
- `void fill(NumericAddressedRegisterInfo& info) const` (modified): writes the
  secondary `offset` into `info.doubleBuffer->address`. It writes no channel:
  `NumericAddressedRegisterInfo::DoubleBufferInfo` has no bar member, and the
  register info already carries the primary address, so the second buffer lies
  on the same channel as the primary. It rejects nothing: `validate()` rejects an
  omitted `secondaryBufferAddress`, one without `offset`, and one that gives `type`
  or `channel`, and the two channels are not compared any more.

### JsonMapFileParser::parse (modified)

Location: src/JsonMapFileParser.cc (modified)

- Rejects the maps that the validity rules of the requirements exclude, without a
  format version bump: its version check stays untouched, while `validate()` owns
  the individual rejections specified above.
- Calls `addInfos` for every top-level entry straight after parsing. That loop and
  the recursion in `addInfos` bind their entries by `auto&` instead of `const auto&`,
  since `addInfos` records the parent link and the path of the sub-entries.
- Keeps the bit-range scan after `addInfos` as it is: `fill` marks a register a bit
  range where its own `representation` or a channel's gives a non-`0` `bitShift`, and
  the scan then marks a register with bit shift `0` and a width below its element
  width where a bit range shares its `bar` and address. Both run before the catalogue
  augmentation from the interrupt handler description
  (`InterruptHandlerEntry::augmentCatalogue`), and the scan compares registers of the
  finished catalogue, so no entry getter can express it.

### Format documentation (modified)

Location: doc/jmapFormat.dox (modified)

- `jmap_access`: a register that gives no `access` inherits the parent's access.
  Such a register also inherits the parent's `triggeredByInterrupt`, unless it gives
  an `access` of its own. A register with an own `address` inherits in the same way.
  A void-typed entry inherits neither `access` nor `triggeredByInterrupt`, while its
  own interrupt reaches its descendants like any other register's.
- `jmap_double_buffering`: `secondaryBufferAddress` is mandatory and gives only its
  `offset`; a `type` or `channel` on it is invalid, and both buffers lie on the same
  channel. A `doubleBuffering` requires a resolved address, since the two buffers lie
  at two addresses, so an entry that gives it and resolves none is rejected (R11).
  Its example and the sentence "with the same members as a regular `address`" drop
  `type`/`channel` accordingly.
- `jmap_representation`: a register that omits its whole `representation` and gives
  no `channels` gets a fixed-point word of its element size, not a 32-bit word, and
  a `representation` that omits `width` gets that width. A `string`
  `representation` always has width `8`, and an own `width` on it is valid only as
  `8`. A channel without a `representation` gets a word of its own element size.
- `jmap_elements`: `numberOfElements` and `bytesPerElement` are inherited from the
  parent where the entry omits its whole `address` and gives no `channels`, with own
  values overriding the inherited ones; otherwise they default to `1` and `4`. A
  value of `0` is invalid. A channel's `bytesPerElement` is its own, never inherited,
  and `4` where omitted. A `pitch` is given only next to `channels` and is never
  inherited.
- `jmap_register` and `jmap_simple_modules`: the register definition becomes "an
  entry that gives `representation` or `channels`, or that resolves an `offset`",
  and the sentence "A register/module in the `addressSpace` without an `address` is a
  module" becomes the rule of `jmap_modules` below; "A module is not a register and
  carries no data of its own" says that no address resolves for a module.
- `jmap_modules`: a module is an entry for which no address resolves on the way up
  to a void entry or a top-level entry, and which gives neither `representation` nor
  `channels`. Every member but `children` is rejected: `access`,
  `triggeredByInterrupt`, `numberOfElements`, `bytesPerElement`, `doubleBuffering`,
  `engineeringUnit`, `description` and `selectedBy` among them. An entry with
  `channels` is a register on the address that resolves for it, and one with
  `representation` or `channels` but no such address is rejected.
- `jmap_address_inheritance`: the `type`, `channel` and `offset` bullets of the
  `address` object. A present `address` always gives `offset`; only `type` and
  `channel` may be omitted. Both are inherited from the parent. If the entry gives
  its own `type`, the inherited `channel` is dropped and an omitted `channel` is
  `0`. Only where no entry above gives an address do they default to `IO`/`0`. The
  inheritance example states that its `STATUS` child inherits `numberOfElements: 2`
  of `APP`, instead of claiming an identical outcome to the example above it.
- `jmap_channel_bitfields`: a bit field gives a `representation` with an optional
  `bitShift`, which is `0` where omitted, and an optional `width` inside the
  channel's element, and may give `engineeringUnit`
  and `description` of its own; the channel supplies its address and element size. A
  bit field without a `representation` is invalid. A child that spans the whole
  element or exceeds it stays ignored, as CR-003 states.
- `jmap_address_inheritance`, `jmap_elements`, `jmap_access`, `jmap_representation`
  and `jmap_modules`: every inheritance section names what is not inherited.
  `representation`, `engineeringUnit`, `description`, `doubleBuffering` and
  `channels` stay empty on a child, which takes the parser's default where the member
  is mandatory.
- `jmap_2d`: a 2D register has `channels`, not `children`, and no register-level
  `representation`, the two being mutually exclusive. A `channels` entry gives
  `pitch`. Its `numberOfElements` is its own and `1` where omitted, since an entry
  with `channels` inherits no geometry. The sentence "Per-channel `selectedBy`
  declarations always override this register-level default" becomes the rule of
  `jmap_channel_selection`: a register-level `selectedBy` reaches every channel that
  has no declaration above it.
- `jmap_channel_selection`: a `selectedBy` is inherited, and an inherited one is
  never overridden, so neither an entry nor a channel below one that declares it may
  give its own. A channel with no declaration above it may give its own, which its
  bit fields take; a bit field never declares one, and neither does a module.
  Neither a channel nor a bit-field child of one may have a void `representation`.
  This
  replaces the "nearest ancestor takes precedence" and "own `selectedBy` overrides"
  statements and the `REG_B` example of `jmap_module_selection_inheritance`, whose
  `MODULE` example becomes a register carrying an `address` and a `representation`
  next to its `children`, since a module gives only `children`.
- `jmap_interrupts`: the void-typed entry forbids an own `address` and takes none
  from its ancestry either, so the address chain below it starts afresh; a
  `doubleBuffering`, `channels`, `numberOfElements`, `bytesPerElement`, `pitch`, an
  own `selectedBy` or a missing `triggeredByInterrupt` on it is invalid. The
  interrupt example stays as it is, its top-level VOID entries having no address.

### JSON schema (modified)

Location: schemas/jmap.schema.json (modified)

- The schema states the rules for external validators; the parser stays the
  enforcement point. The schema is the stricter of the two wherever a per-entry
  validator can decide more than the parser checks - an unknown member, a rule that
  needs no ancestry - so a map can satisfy the parser and fail the schema. A drift
  between the two is caught by review only, as DI-9 records.
- `addressSpaceEntry`: the register and module classification of `jmap_modules`
  above, as description only, since a per-entry schema cannot see whether an address
  or a void entry sits above the entry. That holds for a module giving no member but
  `children` and for a module's `selectedBy` as well: the description states those
  rules, the parser rejects them, because a per-entry schema cannot tell a module from
  a grouping register that inherits its address.
- `numberOfElements` and the `bytesPerElement` definition: the inheritance and
  defaults of `jmap_elements` above, plus `minimum: 1` on both, which covers the
  register member and the channel's; `pitch` already has it.
- `address`: states that only `type` and `channel` are inherited, `offset` being the
  entry's own.
- The 2D-register block drops its `selectedBy: false` and the "must not combine"
  description, since the register-level selector is the default for the channels.
  It gains `"children": false` and `"representation": false`, since a 2D register has
  neither.
- The `type`, `channel` and `selectedBy` fields: the inheritance rules of the
  `address` and `jmap_channel_selection` bullets above, the latter as description
  only, since a per-entry schema cannot see the ancestor; the void-typed entry's
  `allOf` block rejects an own `selectedBy`.
- `width` and the `representation` description: the defaults of
  `jmap_representation` above, including the fixed `8` of a `string` and the
  rejection of an own `width` other than `8` on it.
- `access` and `triggeredByInterrupt`: state the inheritance and the exclusion of the
  pair.
- `channel` and `channelChild`: a `void` `representation` is rejected on both, since
  such a field carries no width to place the range, and a channel requires `offset`.
  The channel-child definition keeps `engineeringUnit`, `description` and
  `representation`, so `additionalProperties: false` rejects every other member
  except the `^_` comments,
  and its description states that the channel supplies the address, the element size
  and the selector. It requires `representation`, which the parser rejects when
  omitted, and describes `bitShift` as defaulting to `0`. It requires no `width`,
  since a bit field that omits one spans the whole element and is ignored, as R5
  states.
- The void-typed entry: the description says that it forbids an own `address` and
  inherits none, so the address chain below it starts afresh, the latter being no
  constraint a per-entry schema can express; `doubleBuffering`, `channels`, the
  geometry members and an own `address` are rejected as new constraints in the
  `allOf` block, and `triggeredByInterrupt` is required.
- `doubleBuffering` gets a `required` list naming `secondaryBufferAddress`, which the
  definition does not have today. `secondaryBufferAddress` gets its own definition
  instead of the `$ref` to `#/definitions/address`: it carries only the required
  `offset` and rejects `type`/`channel` through `additionalProperties`.

### chimeratk-map-to-jmap (modified)

Location: tools/chimeratk-map-to-jmap (modified)

- Writes `type` and `access` on every register, because a converted map would resolve
  them differently below a `DMA`, `RO` or interrupt-bearing parent. The `INTERRUPT`
  guard stays, so an interrupt register keeps writing `triggeredByInterrupt` and no
  `access`, which `validate()` rejects as a pair. `_fill_entry_from_reg` emits it in
  the two branches that write an entry - the 2D branch and the scalar branch - each
  behind that guard, while the void branch emits none, a void-typed entry carrying no
  access. Both branches name it, since each returns before the other.
- Omits a geometry value it cannot supply instead of writing `0`, and writes an
  explicit `representation` with its `width`, except for a `string`, whose `width` it
  leaves out because the parser fixes it at `8`. So no register changes its word by an
  omitted `width`.
- Drops `_ensure_void_parents` and the unused `parent_data` parameter: a top-level
  void-typed entry parses without an address-bearing parent, and the synthetic one
  would become a register under the new rules.

### chimeratk-device-to-jmap (not modified)

Location: tools/src/chimeratk-device-to-jmap.cc

- A module component that is not a register of the device, sitting below an
  address-bearing register, is emitted as a `children`-only node. That node resolves
  the parent's address and becomes a grouping register under R7, so a converted map
  then carries a register the device lacks. The device model permits the shape; no
  test covers it, as DI-11 records.
- It keeps emitting `{"type": "string"}` without a `width`, which now resolves to
  `8` on the parser's side, so it needs no change.

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
  data structure that duplicates what the parent already holds and needs a field for
  every inherited value. With recursive resolution, each rule appears in exactly one
  getter.
- A linking pass that stores the parent pointer before `addInfos`, with a `linked`
  flag and assertions on move and copy: rejected. The descent assigns the pointer
  itself, so no state the assertions guard can arise.
- Holding the parent as `std::shared_ptr` with `enable_shared_from_this`: rejected.
  The child-to-parent direction closes a cycle and needs a `weak_ptr`, each getter
  would lock it, the children container becomes a map of pointers, and every entry
  needs an allocation - all for a tree that lives inside one function call.
- Making the entries non-movable through `std::unique_ptr` sub-entries or a
  hand-written deserialiser for the children map: rejected. Parsing moves each entry
  into its `std::map` node, and both alternatives cost a custom parsing path without
  removing the need for the parent pointer.
- Getters that throw instead of asserting: rejected. A rejection belongs in
  `validate()`, which names the entry through `getPath()`; a throwing getter would
  keep the same check in a second place.
- Requiring a void-typed entry to have an address in its ancestry, as a register is
  located by one: rejected. A void-typed entry carries no data to locate. Taking the
  parent's address instead would make the top-level interrupt example of
  `doc/jmapFormat.dox` an error and force both generators to invent an
  address-bearing parent. Letting it inherit nothing terminates the address chain,
  which leaves its children in the position of top-level entries.
- Inheriting the `channel` even where the entry gives its own `type`: rejected. A
  channel means something different per `type`, so an inherited channel across a
  type change is meaningless.
- Keeping `type`/`channel` on `secondaryBufferAddress`, as the schema `$ref` to
  `address` suggests: rejected. A secondary on another channel is unsupported
  anyway, so the fields could only restate the same channel.
- Inheriting an omitted `secondaryBufferAddress` `offset` from the register:
  rejected. The second buffer would share the primary's address and the two buffers
  would be indistinguishable.
- Making `pitch` inherited like the rest of the geometry: rejected. A `pitch` exists
  only next to `channels`, and a `channels` entry has no `children`, so no entry can
  observe an inherited one.
- Rejecting a bit-range child that spans the whole element or exceeds it, where the
  parser ignores it today: rejected. CR-003 requires the child to be ignored for
  forward compatibility, and a map written for a later format must keep parsing.
- Letting an own `selectedBy` override an inherited one, as `doc/jmapFormat.dox`
  stated until now: rejected. A selected 2D register selects each of its channels,
  and a selected parent selects its children, so an own selector below a declaring
  entry contradicts the selection above it. The parser stored such an override, but
  nothing acts on a selector yet, so no working behaviour is lost.
- Giving a `selectedBy` on a branch only the meaning of a value its leaves inherit,
  while the branch itself stays unselected: rejected. It would resolve the
  contradiction, but the same key would then mean different things depending on
  whether an entry has `children`, which a reader of a map cannot see.
- Making the width of a `string` `representation` follow the element size like a
  fixed-point one: rejected. A character is 8 bits, and `bytesPerElement: 40` would
  otherwise turn a string register into a 320-bit word.
- Making the two generators state the `string` width explicitly instead of giving
  `string` a fixed `8`: rejected. It would force both tools to restate a rule of the
  format.

## Test plan

All parser cases go into `tests/executables_src/testJsonMapFileParser.cpp`. Each
loads `tests/simpleJsonFile.jmap`, or the fixture it names, and applies its mutation
through one shared helper that injects it, writes a temporary file and parses it. The
helper is the generalised form of `parseInjectedInterruptMap`: it takes the base
document as well as the mutation, so it serves the cases this change adds, and
`parseInjectedInterruptMap` stays as a thin wrapper on it, so its existing call sites
need no rewrite. `parseWithMapFormatVersion` stays as it is. No fault-only fixture is
added.

`tests/simpleJsonFile.jmap` is shared with the backend test binaries:
`tests/muxedDataAccessor.jmap` reaches them through `tests/dummies.dmap` and
`tests/logicalnamemap.dmap`, and `tests/groupedDemux.jmap` is read by
`testGroupedDemultiplexing.cpp`. Both edits only drop the `secondaryBufferAddress`
`type`/`channel` keys, which no case asserts. A case
relying on an existing entry names it, so a fixture edit that changes what it inherits
shows up in review. Each case's `Requirement:` line below names the `R` bullets it
covers, and its `Necessary:` line states why no other case covers them; neither
appears in the test source. A case without a status is new. A rejection case expects
`ChimeraTK::logic_error` and nothing more. Each rule has its own case; where a rule
covers several members, or the same member at two levels, one case applies those
mutations to the same document and expects each of them to be rejected.

- `tests/executables_src/testJsonMapFileParser.cpp` (modified)
  - `TestDoubleBufferDiffBarThrows` (removed): its rule goes with `type`/`channel` on
    a `secondaryBufferAddress`, and `tests/simpleJsonFile.diffBar.jmap` exists only
    for it.
  - `TestSelectedByInheritance` (modified): `INHERIT` becomes a register that
    declares the selector next to its `children`, since a module gives only
    `children`; `INHERIT.REG_B`, `INHERIT.REG2D_OWN` and `INHERIT.CHANB.D1` give no
    selector of their own, and every register and channel below `INHERIT` expects the
    selector `INHERIT` declares (`/APP/OUTPUT_SELECT`, 3), the channel `D0`/`D1` pair
    of `CHANB` expecting it twice.
    - Requirement: R8 (`selectedBy` inheritance and no override).
    - Necessary: the case is the only one where one declaration reaches a grouping
      register (`SUB`), a 2D register and a register with children, so it keeps
      covering inheritance after the three overrides go; a channel declaring a
      selector itself is covered by
      `TestNamedChannelBitRangeMuxedChildSelectedBy`, whose channel declares one
      with nothing above it.
  - `TestAddressInheritanceOwnTypeDropsChannel`: a child `address` giving `type` and
    the required `offset`, its `channel` omitted, below a `DMA` address on channel 5;
    the child's own `type` applies and its `channel` is `0`.
    - Requirement: R1 (an own `type` drops the inherited `channel`).
    - Necessary: `BSP.VERSION` and `BSP.SOME_INFO` sit below `BSP`, whose `address`
      gives neither `type` nor a channel other than `0`, so they cannot tell
      inheritance from defaulting; this is the only case with an own `type` below a
      non-default pair.
  - `TestAddressInheritanceOwnChannelKept`: a child `address` giving `channel` and
    the required `offset`, its `type` omitted, below a `DMA` address on channel 5;
    the inherited `type` applies and the `channel` is the entry's own, `2`.
    - Requirement: R1 (an own `channel` is kept, the `type` inherited).
    - Necessary: no other case gives an own `channel`; the inherited `type` is
      observable because the parent's is `DMA`, not the `IO` default.
  - `TestAddressInheritanceInheritsPair`: a child `address` giving only the required
    `offset`, below the same `DMA` address on channel 5, resolves to `DMA` and channel
    `5`.
    - Requirement: R1 (an own `address` may omit `type` and `channel` and inherits
      both).
    - Necessary: the two cases above omit one of the two at a time, so a getter that
      inherits only one of them passes both; this one needs a non-default parent too,
      or `IO`/`0` would hide the inheritance.
  - `TestGoodMapFileParse` (modified): its assertions move to the resolved values
    that the change produces, among them `BSP.SOME_INFO`, whose `representation`
    omits `width`, so its word becomes the `8` bits of a string character.
    - Requirement: R7, R13.
    - Necessary: it is the only case that walks the whole shared fixture, so it is
      where a fixture entry whose resolution changed shows up.
  - `TestChannelsEntryInheritsAddressBecomesRegister`: an entry with `channels` and
    a `pitch`, neither an own `address` nor a `representation`, below an
    address-bearing entry on `DMA` channel 3 at a non-zero offset with
    `numberOfElements: 2` and `bytesPerElement: 8`; it yields a register on that
    address with one element and its `pitch * 8` as the pitch, its channel slices
    resolve to that address and carry the channel's `bytesPerElement`, and a
    bit-field child of one channel, say `bitShift: 8` and `width: 4`, resolves there
    too with a slice of its own. A second channel omits its `bytesPerElement` and
    defaults to `4`, not to the parent's `8`; a third omits its `representation` and
    gets a fixed-point word of its own element size, not the parent's; a fourth gives
    a `representation` with only its `type`, whose `width` likewise becomes its
    element size.
    - Requirement: R5 (a 2D register and its channels), R4 (no geometry inheritance
      with `channels`), R13 (a channel's `representation` without `width`).
    - Necessary: every `channels` entry in the existing maps carries its own
      `address`, so an address-less register passing its address to a channel and on
      to a bit field is untested; the parent's element size differs from the
      channel's default, so a channel that wrongly inherited it fails.
  - `TestGroupingEntryBelowAddressBecomesRegister`: an address-bearing entry on
    `DMA` channel 3 at a non-zero offset, with `bytesPerElement: 8`, `access: RO` and
    `numberOfElements: 3`, whose child gives only `children`, with an address-less
    grandchild carrying a `representation`; the child inherits the address and the
    access and yields a register of its own with a 64-bit fixed-point word and the
    inherited `numberOfElements` of 3, and the grandchild is a `READ_ONLY` register
    on the same address with the same number of elements. A second child of the same
    parent gives neither `representation`, `channels` nor `children` and becomes a
    register of its own.
    - Requirement: R7 (a grouping entry is a register; an entry giving none of the
      three is one as well), R3, R4.
    - Necessary: the existing maps have modules above a register-bearing entry
      (`APP`, `MY_INTC`) and a module without any address (`MY_INTC.SUB0`), but no
      entry below an address that gives only `children`, which the parser rejects
      today, and none that gives neither of the three, which it skips.
  - `TestModuleMembersRejected`: each of `access`, `triggeredByInterrupt`,
    `numberOfElements`, `bytesPerElement`, `doubleBuffering`, `engineeringUnit`,
    `description` and `selectedBy` is given on a module in turn, and each mutation is
    rejected. The `selectedBy` mutation sits on a module below a void-typed entry,
    which passes `INTERRUPT` down, so no other rule rejects that map. `pitch` is not
    among them, since `pitch` without `channels` is rejected on any entry.
    - Requirement: R11.
    - Necessary: `MY_INTC` carries none of the members, so each mutation is needed to
      reach its rejection; the members are independent and no other case reaches the
      descriptive ones.
  - `TestModuleBelowVoidEntryResolvesNoAddress`: an address-bearing register, a VOID
    entry below it, a module below the VOID entry that gives only `children`, and a
    register of its own below that module; the middle entry is a module although an
    address exists further up, since the VOID entry stops the search.
    - Requirement: R11 (no address resolves up to a void-typed entry or a top-level
      entry).
    - Necessary: every other module case has a true module at the top, so a search
      that ran to the top regardless of a VOID entry would pass all of them.
  - `TestGeometryInheritanceAndOwnSampleWidth`: an address-bearing parent with
    `numberOfElements: 2` and `bytesPerElement: 8` and five children: the first
    three are address-less and carry a `representation` - the first inherits both
    values, the second gives `bytesPerElement: 2` and keeps the inherited `2`
    elements, the third gives `numberOfElements: 3`, which supersedes the inherited
    `2` - the fourth gives a `representation` with only its `type`, so its `width`
    defaults to the element size in bits, and the fifth gives an own `address` with
    only its `offset`, which stops the geometry inheritance, so it keeps one element
    of four bytes. Where a child overrides `bytesPerElement`, its value shows in
    `elementPitchBits`, so the inherited and the defaulted value stay apart.
    - Requirement: R4, R6 (an own `bytesPerElement` changes the element size only,
      which `elementPitchBits` shows), R7 (an omitted `width`).
    - Necessary: no case overrides one of the two inherited geometry values below a
      parent with a non-default geometry, and no case gives a `representation`
      without `width` below such a parent; `BSP.VERSION` and `BSP.SOME_INFO` inherit
      from `BSP`, but only a child that overrides one value shows the inherited other
      one.
  - `TestChannelsWithOwnRepresentationRejected`: a 2D register that also gives a
    `representation` is rejected.
    - Requirement: R5 (`channels` and `representation` are mutually exclusive).
    - Necessary: no fixture gives both, and the parser ignored the `representation`.
  - `TestVoidRepresentationOnChannelsRejected`: a channel of a 2D register whose
    `representation` is `void`, and a bit-field child of a channel with a void
    `representation`, are rejected, each as its own mutation.
    - Requirement: R5.
    - Necessary: no fixture has a void channel or bit field, and `Channel::fill`
      would emplace a VOID channel with a width instead of rejecting it.
  - `TestBitFieldWithoutRepresentationRejected`: a bit-field child of a channel that
    gives no `representation` is rejected.
    - Requirement: R5.
    - Necessary: no fixture omits one, and `makeChannelInfo` turned such a child into
      a zero-width slice until now.
  - `TestTwoDimensionalEntryWithChildrenRejected`: a 2D register that also gives
    `children` is rejected.
    - Requirement: R5.
    - Necessary: no fixture gives both, and the parser iterated the `children`.
  - `TestChildOfBitRangeRegisterNotABitRange`: a register with a non-`0` `bitShift`
    and an address-less child carrying a full-element-width `representation`; the
    child is no bit range of its own.
    - Requirement: R13.
    - Necessary: no fixture has a bit-range register with children, and the base
      copied the parent register's `isBitRange` into the child, so every other case
      passes either way.
  - `TestStringWidthRejected`: a `string` `representation` that gives a `width`
    other than `8` is rejected.
    - Requirement: R7, R12.
    - Necessary: a string's width is fixed at `8`, and a map that states any other
      width must not be overridden silently.
  - `TestChannelsEntryWithoutPitchRejected`: a `channels` entry without `pitch` is
    rejected.
    - Requirement: R5.
    - Necessary: the schema requires `pitch` next to `channels`, the parser accepted
      its absence silently.
  - `TestPitchWithoutChannelsRejected`: an own `pitch` on a register that gives no
    `channels` is rejected.
    - Requirement: R5.
    - Necessary: the other direction of the same rule, and no fixture gives a
      `pitch` without `channels`.
  - `TestAddressWithoutOffsetRejected`: an entry below a parent with an `offset`
    that gives its own `address` without one is rejected.
    - Requirement: R1.
    - Necessary: no existing map has an address without `offset`.
  - `TestChannelWithoutOffsetRejected`: a channel of a 2D register with its `offset`
    removed is rejected.
    - Requirement: R5.
    - Necessary: `Channel::offset` is uninitialised today, so no fixture reaches the
      rejection, and the schema's requirement was unenforced by the parser.
  - `TestDoubleBufferingAddressRejected`: a `doubleBuffering` that omits
    `secondaryBufferAddress`, one whose `secondaryBufferAddress` omits `offset`, and
    one whose `secondaryBufferAddress` gives a `type` or a `channel`, are rejected,
    each as its own mutation.
    - Requirement: R2.
    - Necessary: every existing map gives a bare `secondaryBufferAddress` with a
      `type`, so none of the three is reached today; they are one rule over one
      object.
  - `TestAccessInheritanceWithOwnAddress`: an address-bearing parent with `access:
    RO`, a `representation` of its own (`IEEE754`, `width: 64`), `engineeringUnit`,
    `description` and a `doubleBuffering`, and two children whose own `address`
    carries only `offset`; both are `READ_ONLY`. The first gives its own
    `representation` and is not reinterpreted by the parent's; the second omits it
    and gets the defaulted fixed-point word, not the parent's. The two children leave
    `engineeringUnit`, `description` and `doubleBuffering` empty, so none of the three
    is inherited.
    - Requirement: R3, R9.
    - Necessary: no fixture combines an own `address` with an omitted `access` below
      a parent whose access is not the default, and nothing checks that the three
      descriptive and buffer members stay off a child.
  - `TestTriggeredByInterruptInheritance`: an address-bearing parent with
    `triggeredByInterrupt` and five children; the address-less one without `access`
    is an `INTERRUPT` register with the parent's interrupt id, the address-less one
    giving `access: RO` is `READ_ONLY` without an interrupt id, the one whose own
    `address` gives only `offset` is `INTERRUPT` as well, a void-typed child with its
    own `triggeredByInterrupt` keeps that id instead of the parent's, and a child
    below a parent with `access: RO` that gives a `triggeredByInterrupt` of its own is
    `INTERRUPT` rather than `READ_ONLY`.
    - Requirement: R3.
    - Necessary: no fixture reaches this rule, since no interrupt register in the
      fixture has address-less children; `DAQ.CTRL` and `DAQ.FD` carry
      `triggeredByInterrupt` but stand alone.
  - `TestTriggeredByInterruptPassedThrough`: a parent with `triggeredByInterrupt`, an
    address-less child with a `representation` and no interrupt of its own, and a
    grandchild with a `representation`; the grandchild carries the parent's id.
    - Requirement: R3 (the chain, not only the direct parent).
    - Necessary: every case of `TestTriggeredByInterruptInheritance` has a
      declaring parent directly above, so a getter reading the parent's own
      declaration instead of its resolved value would pass.
  - `TestTriggeredByInterruptInheritsName`: an address-bearing parent whose
    `triggeredByInterrupt` names another interrupt register of the fixture, and an
    address-less child with a `representation`; both resolve to that register's id
    after the parse, which
    `NumericAddressedRegisterCatalogue::finaliseInterruptBookkeeping()` performs.
    - Requirement: R3.
    - Necessary: CR-020 lets the field name a register; the declaration is a variant
      and nothing covers a name inherited from a parent.
  - `TestAccessAndTriggeredByInterruptRejected`: an entry that gives both `access`
    and `triggeredByInterrupt` is rejected.
    - Requirement: R3.
    - Necessary: the check moves from `fill` into `validate()`, and no test reaches
      it today.
  - `TestRegisterWithoutAncestorAddressRejected`: an entry with `channels` and a
    `pitch`, and an entry with a `representation`, each with no address anywhere in
    its ancestry, are rejected. The `pitch` is given so that the `channels` case is
    not caught by the `channels`-without-`pitch` rule.
    - Requirement: R1.
    - Necessary: no existing map has a register without an address in its ancestry,
      and the two entries cover the two members that make an entry a register.
  - `TestZeroGeometryRejected`: `numberOfElements: 0` and `bytesPerElement: 0` on an
    otherwise valid register, a channel's `bytesPerElement: 0` and a `pitch: 0` on a
    2D register are rejected, each as its own mutation.
    - Requirement: R4.
    - Necessary: `0` means "not set" today, so the new reading needs its own case;
      the register, the channel and the pitch are the three places the rule applies.
  - `TestDoubleBufferingWithInheritedAddress`: a `doubleBuffering` register without
    an own `address`, below an address-bearing entry on `DMA` channel 3 at a non-zero
    offset, gets its `BUF0` view on the resolved address and its `BUF1` view on the
    address from `secondaryBufferAddress`, both on the same channel.
    - Requirement: R2, R1.
    - Necessary: every existing `doubleBuffering` gives its own `address`, so an
      inherited one with its views is untested.
  - `TestSelectedByOverrideRejected`: an entry below a `selectedBy`-declaring parent
    that gives its own `selectedBy` is rejected; so is a mutation that gives the
    channel `INHERIT.CHANB.D1` an own selector.
    - Requirement: R8 (never overridden).
    - Necessary: no fixture entry with `children` declares a selector, and no channel
      below a declaring entry gives one, so both halves need a mutation; the mutations
      are exactly the declarations this change removes from the fixture.
  - `TestChannelSelectedByOnWritableRegisterRejected`: a 2D register with `access:
    RW` whose channel declares its own `selectedBy` is rejected.
    - Requirement: R8 (a selector needs `READ_ONLY` or `INTERRUPT` access).
    - Necessary: the base rejected it by looping over the register's channels, which
      `Channel::validate` takes over; the own and the inherited selector on a
      register stay covered by `TestSelectedByReadWriteRejected` and
      `TestSelectedByInheritedWritableDescendantRejected`.
  - `TestVoidEntryWithDoubleBufferingRejected`: a VOID-typed entry that gives
    `doubleBuffering` is rejected.
    - Requirement: R10.
    - Necessary: no fixture VOID entry gives one, and the parser ignored it today.
  - `TestVoidEntryWithChannelsRejected`: a VOID-typed entry that gives `channels` and
    a `pitch` is rejected.
    - Requirement: R10.
    - Necessary: no fixture VOID entry gives `channels`.
  - `TestVoidEntryWithAddressRejected`: a VOID-typed entry that gives an own
    `address` is rejected.
    - Requirement: R10, R12.
    - Necessary: every fixture VOID entry has none, and the rejection moves from
      `fill` into `validate()`; no case reaches it today.
  - `TestVoidEntryWithoutInterruptRejected`: a VOID-typed entry without its own
    `triggeredByInterrupt` is rejected.
    - Requirement: R10, R12.
    - Necessary: R10 makes the member mandatory, and no case reaches the rejection.
  - `TestVoidEntryGeometryRejected`: VOID-typed entries giving an own
    `numberOfElements` and `bytesPerElement` are rejected, each as its own mutation.
    - Requirement: R10.
    - Necessary: no fixture VOID entry gives either of the two; `pitch` is not among
      them, since `pitch` without `channels` is rejected on any entry.
  - `TestVoidEntrySelectedByRejected`: a VOID-typed entry that gives an own
    `selectedBy` is rejected.
    - Requirement: R10.
    - Necessary: the selectedBy half of the void rule, separate from the geometry.
  - `TestVoidEntryTakesNoAddress`: a VOID-typed entry below an address-bearing parent
    on `DMA` channel 3 at a non-zero offset is a register with `address` 0,
    `nElements` 0, `bar` 0 and `elementPitchBits` 0, the last two being what the base
    copied from the parent register.
    - Requirement: R10 (no inherited address).
    - Necessary: the VOID entries of the fixture (`BSP.VOID_INTERRUPT_0` and its
      sibling `BSP.VOID_INTERRUPT_3_0_1`) sit below an address-bearing entry and only
      their `pathName` is asserted today.
  - `TestVoidEntryChildren`: a VOID entry below a parent on `DMA` channel 3 that
    gives `numberOfElements: 2` and `bytesPerElement: 8`, with one child that gives a
    `representation` but no own `address` and is rejected, since the VOID entry
    passes none on; a sibling whose `address` gives only `offset` and which gives no
    `access` parses as an `INTERRUPT` register with the VOID entry's id, resolved to
    `IO` and channel `0`, which only the default behind the VOID entry can supply, and
    with one element of four bytes, not the parent's geometry; a `selectedBy`
    declared above the VOID entry reaches it.
    - Requirement: R10, R1, R8.
    - Necessary: no fixture has children below a VOID entry, so neither the
      rejection nor the default behind a VOID entry, nor the geometry it does not pass
      on, is reached elsewhere.
  - `TestTopLevelVoidEntryParses`: a top-level VOID entry with its own
    `triggeredByInterrupt`, injected into `tests/simpleJsonFile.jmap`, parses.
    - Requirement: R10.
    - Necessary: no fixture has a top-level VOID entry, and the interrupt example of
      `doc/jmapFormat.dox` is a document, not a fixture.
- `tests/simpleJsonFile.diffBar.jmap` (removed)
  - The fixture exists only for the removed case
    `TestDoubleBufferDiffBarThrows`, which asserted the rejection of a
    `secondaryBufferAddress` whose `bar` differs from the primary's.
- `tests/simpleJsonFile.jmap` (modified)
  - Loses the `type`/`channel` of every `secondaryBufferAddress`, which R2 rejects.
- `tests/muxedDataAccessor.jmap` (modified)
  - Loses the `type`/`channel` of every `secondaryBufferAddress`; its
    `addressSpace.TEST.children.DMA` tree stays as `TestAllTwoDimensionalSkipped`
    of `testDeviceToJmap.cc` reads it.
- `tests/groupedDemux.jmap` (modified)
  - Loses the `type`/`channel` of every `secondaryBufferAddress`.
- `tests/selectedByInheritance.jmap` (modified)
  - Loses the own `selectedBy` of `INHERIT.REG_B`, `INHERIT.REG2D_OWN` and the
    channel `INHERIT.CHANB.D1`, and the comments that call them overrides; the
    channel `CHANA.C0`/`C1` and `CHANB.D0` keep inheriting.
  - `INHERIT` gains an `access`, an `address` and a `representation`, becoming the
    register that declares the subtree's selector, and its comment stops calling the
    declaration module-level.
- Existing cases adapted, not extended:
  - `TestNamedChannelDirectBitRange` keeps passing: channel `B` of
    `tests/bitRangeChannels.jmap` stays a bit range of its register, since a channel
    slice still takes `isBitRange` from the channel's own `bitShift`.
  - `TestSelectedByReadWriteRejected` and
    `TestSelectedByInheritedWritableDescendantRejected` keep their rule, which moves
    from `fill` into `validate()`; the helper `parseSelectedByInheritedAccessFixture`
    gives its `MOD` an `access: RO` so it is a register carrying children rather than
    a module, which R11 would reject.
  - `TestIsrChildWithChannels` and `TestIsrWithChannels` inject `channels` without a
    `pitch`, which the new rejection catches first; both get a `pitch` so they keep
    testing the ISR-child rule they exist for.
  - `TestIsrDoubleBuffered` gives `type`/`channel` in its injected
    `secondaryBufferAddress`, which the same rejection catches first; it keeps only
    the `offset`.
  - `TestNamedChannelBitRangeMuxedChildSelectedBy` keeps passing: a bit field still
    takes the selector of its channel, and `DAQ.MUXED_WITH_STATUS` declares none
    above it.
  - `TestNamedChannelBitRangeIgnoredChildren` keeps passing: the whole-element and
    the overflowing child of `tests/bitRangeChildCases.jmap` stay ignored, since
    CR-003 requires it.
- `tests/executables_src/testDeviceToJmap.cc` (not modified)
  - Its round trips keep passing for the devices they cover; the module-below-register
    shape of DI-11 is not among them.

## Deferred issue

- DI-1 [ACCEPTED] `chimeratk-map-to-jmap` has no test harness, so nothing verifies
  the maps it emits under the rules of this change; its output shape is checked by
  review only.
- DI-2 [FOLDED-IN] The nested `Address` type cannot carry a resolved address, since
  its fields are optional; the resolution stays in `getOffset()` and
  `getTypeAndChannel()`, which form the values on each call.
- DI-3 [FOLDED-IN] `validate()` cannot tell a bit field or its channel's element
  width from the entry's own members; folded in as `Channel::validate()`, which the
  entry's `validate()` calls once per channel.
- DI-4 [FOLDED-IN] `DoubleBufferingInfo::fill` cannot write a bar, since
  `NumericAddressedRegisterInfo::DoubleBufferInfo` has no such member.
- DI-5 [FOLDED-IN] The specified `Address::fill` is superseded by writing the
  resolved address directly in `fill`.
- DI-6 [FOLDED-IN] The bit-field rejection left the effective width of an omitted
  `width` undefined; folded in as the ignore rule of CR-003, with `width` optional and
  `bitShift` defaulting to `0`.
- DI-7 [FOLDED-IN] `makeChannelInfo` needs the resolved width, which the plan's
  signature does not carry.
- DI-8 [ACCEPTED] CR-016 (`mhier-agent/CR-016-runtime-selectedby-enforcement`)
  renames `selectedBy` to `gatedBy`, which collides with the `selectedBy` rules here,
  and states that the 2D register-level gate is not inherited by channel slices,
  which contradicts R8. It must therefore depend on this change and be rebased onto
  it; the `Depends on: CR-018` line belongs into CR-016, not into this document.
- DI-9 [ACCEPTED] Neither `schemas/jmap.schema.json` nor its agreement with the
  parser is tested, so every rule that only the schema states rests on review, as
  DI-1 records for the tool.
- DI-10 [ACCEPTED] No case asserts a rejection message, since every rejection case
  expects `ChimeraTK::logic_error` and nothing more. `getPath()` and
  `getSelectedBySource()` are therefore exercised only by the parse succeeding, and
  their use in the messages rests on review.
- DI-11 [ACCEPTED] A module nested below a register cannot be expressed in the map
  format: the entry resolves the parent's address and becomes a grouping register.
  `chimeratk-device-to-jmap` therefore emits a register the device lacks for a device
  whose path has such a module component, and no test covers that shape.
- DI-12 [ACCEPTED] `doc/jmapFormat.dox` is not read by any test, so the half of R14
  that concerns it rests on review, as DI-9 records for the schema.
- DI-13 [ACCEPTED] An unknown `access` string, `INTERRUPT` among them, falls back to
  the first listed value of `NLOHMANN_JSON_SERIALIZE_ENUM`, so a map writing
  `"access": "INTERRUPT"` is read as `READ_ONLY` rather than rejected.
