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
  - `secondaryBufferAddress` is mandatory and gives only `offset`, which is required.
  - A `type` or `channel` on it is rejected. The second buffer lies on the same
    channel as the primary, and the parser derives that from the primary address.
- R3: `access` and `triggeredByInterrupt`
  - `access` is inherited unless the entry gives `access` or `triggeredByInterrupt`.
  - `triggeredByInterrupt` is inherited together with the `INTERRUPT` access it
    implies; a `triggeredByInterrupt` of its own likewise gives the entry that access.
  - An own `access` replaces the pair: the entry keeps its own access and inherits no
    `triggeredByInterrupt`, so no interrupt id reaches it.
  - An entry must not give both `access` and `triggeredByInterrupt`.
- R4: `numberOfElements` and `bytesPerElement`
  - Inherited only where the entry omits its whole `address` and gives no `channels`;
    otherwise own values apply, `1` and `4` where omitted.
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
  - A channel without a `representation` gets a fixed-point word of its own element
    size, as a register does under R7.
  - The `children` of a channel are bit fields of its element. A bit field gives only
    a `representation`; its address and element size are the channel's. A further
    member is not the parser's check: the schema rejects it.
  - A bit field gives `bitShift`; its `width` may be omitted and is then the element
    width. A child that spans the whole element or whose `bitShift + width` exceeds
    it is ignored, with no slice for it, as CR-003 requires for forward
    compatibility.
  - A void `representation` on a channel or a bit field is rejected.
- R6: a `pitch` is never inherited: only an entry with `channels` gives one, and such
  an entry has no `children`. An own `bytesPerElement` changes the element size only.
- R7: a register that gives neither `representation` nor `channels` gets a fixed-point
  word at its resolved address whose width is its element size in bits; a
  `representation` that omits `width` takes that width as well. A `string`
  `representation` is the exception: its width is `8`, the size of one ASCII
  character, whatever the element size, since a character width in bits is no
  property of the map. Every entry that
  gives neither `representation` nor `channels` and resolves an `offset` is such a
  register, a grouping entry among them, and the parser emits it in addition to its
  children.
- R8: `selectedBy`
  - `selectedBy` is inherited.
  - An inherited `selectedBy` is never overridden: neither an entry, a channel nor a
    bit field below one that declares `selectedBy` may give its own.
  - A channel with no declaration above it may give its own selector, which its bit
    fields take. A bit field never declares one.
- R9: `representation`, `engineeringUnit`, `description`, `doubleBuffering` and
  `channels` are never inherited.
- R10: a void-typed entry
  - It holds no data, so `channels`, an own `address`, `numberOfElements`,
    `bytesPerElement`, `pitch` and `doubleBuffering` are rejected.
  - It must give its own `triggeredByInterrupt`, because it inherits neither `access`
    nor an interrupt id.
  - It has no `selectedBy` of its own, while an inherited one passes through it.
  - It resolves no address, whatever its ancestors give, so its children are in the
    position of top-level entries.
  - Its own `triggeredByInterrupt` reaches its descendants like the value of any
    other register.
- R11: a module
  - It resolves no address on the way up to a void-typed entry or the top, and gives
    neither `representation` nor `channels`.
  - It holds no data, so `numberOfElements`, `bytesPerElement`, `pitch` and
    `doubleBuffering` are rejected, and `access` and `triggeredByInterrupt` as well,
    since a module creates no register to carry them.

Aspect: documentation, format version and tools.

- R12: `doc/jmapFormat.dox` and `schemas/jmap.schema.json` state every rule above,
  each rule in the section of the member it concerns.
- R13: The format version stays `1.0`, so the rules apply to every file that declares
  it. A map that violates them stops parsing, among them one that
  - gives an own `address` without `offset`, or gives a channel without `offset`,
  - gives an own `selectedBy` below an entry that declares one, on a channel or on a
    bit field as well,
  - gives `type` or `channel` on a `secondaryBufferAddress`,
  - gives `channels` and a register-level `representation` together,
  - gives `children` next to `channels`,
  - gives a data member on a module,
  - gives `pitch` without `channels`, or `channels` without `pitch`,
  - gives `numberOfElements`, `bytesPerElement` or `pitch` as `0`,
  - gives a `width` on a `string` `representation`, which always has `8`,
  - gives a bit field without `bitShift`,
  - gives a void `representation` on a channel or on a bit field,
  - gives `access` and `triggeredByInterrupt` together,
  - gives a void-typed entry an own `address`, `channels`, its geometry,
    `doubleBuffering` or `selectedBy`, or omits its `triggeredByInterrupt`.
- R14: The following maps parse today but behave differently after the change: they
  resolve differently, or they stop parsing as R13 lists. These differences are
  accepted, so no migration follows:
  - a register with its own `address` and no `access` below a parent with a
    non-`READ_WRITE` access takes the inherited access and interrupt,
  - an own `address` with an omitted `type` below a parent on another channel takes
    the parent's `type`, so the register moves to another `type`/`channel`
    combination,
  - an address-less entry below a parent with a geometry takes the parent's
    `numberOfElements`, `bytesPerElement` and with them its element size and pitch,
    not its own defaults,
  - an entry that gives neither `representation` nor `channels` becomes a register
    once an `offset` resolves for it, which the parser ignored until now,
  - an address-less entry with `channels` is accepted and gets its slices,
  - a `secondaryBufferAddress` without `type` or `channel` is accepted,
  - a `string` `representation` gets width `8` instead of 32 bits, whether it omits
    its `width` or gives one, and a map that gives one stops parsing as R13 lists,
  - a register without a `representation` gets a word of its effective width, not
    32 bits, and a given `representation` that omits `width` gets its element size in
    bits instead of 32,
  - a channel without a `representation` gets a word of its own element size instead
    of a 32-bit word, and so does a channel's `representation` that omits `width`,
  - a void-typed entry writes no address and `elementPitchBits` 0, where it took the
    parent register's address and pitch from the catalogue.
- R15: `chimeratk-map-to-jmap` emits maps that satisfy the rules above:
  - an `offset` in every `address`, an explicit `type` from `_make_address` for every
    bar and not only a DMA one, and an explicit `access` on every register except an
    interrupt register,
  - no zero geometry: a member it cannot give a value for is omitted, not written as
    `0`,
  - an explicit `representation` width wherever omitting it would change the
    resolved width.

## Specifications

The parse runs in three steps, kept apart. The JSON deserialises into the entry
objects, `validate()` then rejects every invalid entry in one place, and only then
`fill()` copies the data into the register info, taking each value from a getter.
The getters resolve inheritance and never reject: they assume valid entries, which
`validate()` has established, and `addInfos` has linked their parents beforehand.
The parser rejects a defined key in an invalid place and ignores a key that no
definition lists; the schema forbids both.

A slice is the read-only register the parser creates for one channel, or for one bit
field of a channel, of a 2D register, and which the catalogue exposes beside the 2D
register itself.

### JsonAddressSpaceEntry (modified)

Location: src/JsonMapFileParser.cc (modified)

- `const JsonAddressSpaceEntry* _parent` (new): the direct parent entry, and
  `nullptr` at the top level.
  - The descent assigns it on every child where it recurses, so a getter always
    finds a complete chain. Neither a linking pass nor a "linked" flag is needed.
  - The pointer refers into the tree of entries. A parent owns its sub-entries
    through `std::map` nodes, so it outlives its child, and no pointer leaves the
    tree: `addressSpace` is a local of the parse function and only copies of
    `NumericAddressedRegisterInfo` leave it.
  - The reader builds every entry in place in its `std::map` node before the descent
    starts, and nothing copies or moves a linked entry afterwards, so no copy can
    carry a stale `_parent`.
- Data members of `JsonAddressSpaceEntry` itself become private and carry a leading
  underscore, and every getter starts with `get`, so a member and its getter cannot
  share a name. The nested JSON shapes (`Address`, `Representation`, `Channel`,
  `ChannelChild`, `DoubleBufferingInfo`) are not reached by a getter and keep their
  names and their macro reader. All inheritance
  is resolved by `const` getters that consult `_parent`. The only non-`const` steps
  are the descent recording `_fullPath` and each child's parent link. Each rule is
  stated in one getter only, no resolved value is threaded down the parsing
  recursion, and no getter rejects.
  - The class body maps the JSON keys itself, since
    `NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT` derives a key from the member
    name, which carries a leading underscore. `from_json` names every key once, and
    a key the map omits keeps the member's default; `to_json` writes the same keys
    and is kept for symmetry only, since nothing serialises an entry.
  - `_numberOfElements`, `_bytesPerElement` and `_pitch` become
    `std::optional<size_t>`, so an own value is told from an omitted one and a
    module can reject them.
  - `_triggeredByInterrupt` keeps CR-020's optional `InterruptId`, so the getters
    pass a register name and a numeric array on alike.
  - `_access` becomes `std::optional<Access>`, and `Access::accessNotSet` is
    deleted. Its value collided with
    `NumericAddressedRegisterInfo::Access::INTERRUPT`; with the sentinel gone, the
    getter expresses the interrupt case with the catalogue's enum.
  - `_parent` and `_fullPath` are not among the keys, so they are neither read from
    nor written to JSON.
  - A value the map does not give is `std::nullopt` where the member is optional and
    `representationNotSet` for the `representation`; `0` never stands for an absent
    value.
- `const RegisterPath& getPath() const` (new): `_fullPath`, the fully qualified path
  of the entry. `_fullPath` (new) is recorded by `addInfos` from its `name` and
  `parentName` arguments, with the dotted alt separator as today, before it calls
  `validate()`, so every rejection message names the entry.
  - It replaces the `name` and `parentName` arguments of `fill`, and is what the
    error messages and `info.pathName` use.
  - It needs no second lookup to name the entry that declared the effective
    selector.
- `std::optional<Address> _address` in `JsonAddressSpaceEntry`, so an omitted
  `address` is a `std::nullopt` and an `address: {}` is a present value whose fields
  are all omitted.
- `bool isRegister() const` (new): true where the entry gives a `representation` (a
  void one included), gives `channels`, or resolves an `offset`. An entry with none
  of the three is a module.
- `std::optional<HexValue> getOffset() const` (new): the own `offset`, else the
  parent's, so the search continues up the chain; `std::nullopt` at the top.
  - A void-typed entry returns `std::nullopt` whatever the parent gives, so it is
    the one register without a resolved address, writes none into the register info
    and passes none on: the search of a child stops at it.
- `std::pair<AddressType, size_t> getTypeAndChannel() const` (new): the resolved
  pair, formed on each call from the entry and its parent, never stored.
  - An own `address` starts from the parent's pair, or `IO`/`0` where the parent has
    no address, and an own `type` replaces the pair by its `type` and channel `0`.
  - An own `channel` replaces the inherited one.
  - Without an own `address` the entry returns the parent's pair.
  - A void-typed entry returns `IO`/`0` whatever the parent gives, so a child that
    brings its own `address` starts from the default pair.
- `Representation effectiveRepresentation(const std::optional<Representation>&
  given, size_t elementBits) const` (new): the given representation with its omitted
  `width` set to `elementBits`, or `8` for a `string`; where the entry gives none, a
  fixed-point representation of `elementBits`. `fill` and the slice creation both
  use it, so the width default of R7 exists once and not per call site.
- `NumericAddressedRegisterInfo::Access getAccess() const` (new): the own `access`.
  Otherwise `INTERRUPT` where `triggeredByInterrupt` is present, whether a name or an
  array. Otherwise the parent's value, and `READ_WRITE` at the top. The catalogue's
  enum is the return type because it has an `INTERRUPT` value; the own `access`
  member stays the parser's `Access`, converted here once.
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
- `std::optional<SelectedBy> getSelectedBy() const` and `RegisterPath
  getSelectedBySource() const` (new): the own selector, else the parent's, for a
  module as well. A void-typed entry never declares one, and `validate()` has shown
  that the entry declares none below an entry that declares one, so an inherited
  selector is never overridden.
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
  - It applies the register-level selector through
    `JsonAddressSpaceEntry::applyRegisterSelectedBy`, fed with `getSelectedBy()`,
    where the entry gives no `channels` and is no VOID. The helper takes the
    selector to apply and no longer chooses between an own and an inherited one.
    Next to `channels`, `fill` hands the same `getSelectedBy()` to `Channel::fill`
    and to the channel slices, so `getSelectedBySource()` feeds the read-only
    message for them as well.
  - It writes the register's `representation` through `effectiveRepresentation`
    below, which supplies the fixed-point word of an entry that gives none and the
    `width` an own `representation` omits. `validate()` has rejected one next to
    `channels`, so `Representation::fill` never appends a channel to a 2D register.
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
    - a `numberOfElements`, `bytesPerElement` or `pitch` of `0` on an entry,
    - what `Channel::validate` rejects for each channel and its bit fields,
      which `validate()` calls once per channel because only the channel knows its
      element width and its `children`,
    - the read-only selector that `fill` rejects today,
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
    `parentName` arguments, so the existing empty-name check stays here.
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
    address shifted by the channel `offset`, `isBitRange` where the bit field's
    `bitShift` is not `0`, and the collision check that rejects a slice path which
    already exists.
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
    and the hand-written reader of `Channel` keeps the same key names as the class's,
    with an absent key leaving the optional at `std::nullopt`.
- `Channel::selectedBy` stays: a channel below no declaring entry may give its own
  selector, and `validate()` rejects only an override. `Channel::fill` takes the
  selector to apply and the entry that declared it from `fill`, and no longer
  chooses between an own and an inherited one.
- A channel without a `representation` gets a fixed-point `Representation` whose
  `width` is its element size in bits, from the same code path that serves the
  register.
- `void validate(const RegisterPath& path, const RegisterPath& selectedBySource)
  const` (new): rejects, for this channel, an omitted `offset`, a `bytesPerElement`
  of `0`, a void `representation`, and an own `selectedBy` where
  `selectedBySource` is not empty, which is exactly the case of a declaration
  above; and for each of its bit fields an omitted `representation`.
  - The channel is the only place that knows an element width and its own
    `children`, so `JsonAddressSpaceEntry::validate()` calls this once per channel
    instead of identifying bit fields by their position in the entry tree.
- A bit field gives only a `representation`; address, element size and pitch are the
  channel's.
  - `ChannelChild` keeps only `representation`. The parser therefore ignores a
    further member on it, and the schema's `additionalProperties: false` rejects it
    and admits the `_`-prefixed comments.
  - Its slice is a copy of the channel slice with `engineeringUnit` and `description`
    cleared, since R9 keeps both off the bit field.
  - The skip of a bit field that spans the whole element or exceeds it stays, as
    CR-003 requires: no slice is created for it and the parse continues.

### JsonAddressSpaceEntry::Representation (modified)

Location: src/JsonMapFileParser.cc (modified)

- `_width` becomes `std::optional<size_t>`, so an own width is told from an omitted
  one; the member initialiser no longer supplies `32`.
- `Representation::fill` no longer recurses into `channels`: `fill` supplies an
  explicit representation for every register and every channel, and writes
  `info.channels` itself.
- A `string` type has width `8`, and `validate()` rejects an own `width` on it.
- The nested type keeps `WITH_DEFAULT`, since its keys carry no leading underscore
  and the macro reads them directly.

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

### Format documentation (modified)

Location: doc/jmapFormat.dox (modified)

- `jmap_access`: a register that gives no `access` inherits the parent's access.
  Such a register also inherits the parent's `triggeredByInterrupt`, unless it gives
  an `access` of its own. A register with an own `address` inherits in the same way.
  A void-typed entry inherits neither `access` nor `triggeredByInterrupt`, while its
  own interrupt reaches its descendants like any other register's.
- `doubleBuffering`: `secondaryBufferAddress` is mandatory and gives only its
  `offset`; a `type` or `channel` on it is invalid, and both buffers lie on the same
  channel. Its example and the sentence "with the same members as a regular
  `address`" drop `type`/`channel` accordingly.
- `jmap_representation`: a register that omits its whole `representation` and gives
  no `channels` gets a fixed-point word of its element size, not a 32-bit word, and
  a `representation` that omits `width` gets that width. A `string`
  `representation` always has width `8`, and one that gives a `width` is invalid. A
  channel without a
  `representation` gets a word of its own element size.
- `jmap_elements`: `numberOfElements` and `bytesPerElement` are inherited from the
  parent where the entry omits its whole `address` and gives no `channels`, with own
  values overriding the inherited ones; otherwise they default to `1` and `4`. A
  value of `0` is invalid. A channel's `bytesPerElement` is its own, never inherited,
  and `4` where omitted. A `pitch` is given only next to `channels` and is never
  inherited.
- `jmap_modules`: a module is an entry for which no address resolves on the way up
  to a void entry or the top, and which gives neither `representation` nor
  `channels`. A module may give `children`, `selectedBy`, `engineeringUnit` and
  `description`; `access`, `triggeredByInterrupt`, `numberOfElements`,
  `bytesPerElement`, `pitch` and `doubleBuffering` on it are rejected. An entry with
  `channels` is a register on the address that resolves for it, and one with
  `representation` or `channels` but no such address is rejected.
- `jmap_address_inheritance`, the `type`, `channel` and `offset` bullets of the
  `address` object: a present `address` always gives `offset`; only `type` and
  `channel` may be omitted. Both are inherited from the parent. If the entry gives
  its own `type`, the inherited `channel` is dropped and an omitted `channel` is
  `0`. Only where no entry above gives an address do they default to `IO`/`0`.
- `jmap_channel_bitfields`: a bit field gives only a `representation`, with
  `bitShift` and an optional `width` inside the channel's element; the channel
  supplies its address and element size. A bit field without a `representation` or
  without `bitShift` is invalid. A child that spans the whole element or exceeds it
  stays ignored, as CR-003 states.
- Every inheritance section names what is not inherited: `representation`,
  `engineeringUnit`, `description`, `doubleBuffering` and `channels` stay empty on a
  child, which takes the parser's default where the member is mandatory.
- `jmap_2d`: a 2D register has `channels`, not `children`, and no register-level
  `representation`, the two being mutually exclusive. A `channels` entry gives
  `pitch`. Its `numberOfElements` is its own and `1` where omitted, since an entry
  with `channels` inherits no geometry.
- `jmap_channel_selection`: a `selectedBy` is inherited, and an inherited one is
  never overridden, so neither an entry, a channel nor a bit field below one that
  declares it may give its own. A channel with no declaration above it may give its
  own, which its bit fields take. This replaces the "nearest ancestor takes
  precedence" and "own `selectedBy` overrides" statements and the `REG_B` example of
  `jmap_module_selection_inheritance`.
- `jmap_2d` and `jmap_channel_selection`: the sentence "Per-channel `selectedBy`
  declarations always override this register-level default" becomes the rule above:
  a register-level `selectedBy` reaches every channel that has no declaration above
  it. Neither a channel nor a bit-field child of one may have a void
  `representation`.
- The void-typed entry forbids an own `address` and takes none from its ancestry
  either, so the address chain below it starts afresh; a `doubleBuffering`,
  `channels`, its geometry, an own `selectedBy` or a missing `triggeredByInterrupt`
  on it is invalid. The interrupt example stays as it is, its top-level VOID entries
  having no address.
- The inheritance example states that its `STATUS` child inherits
  `numberOfElements: 2` of `APP`, instead of claiming an identical outcome to the
  example above it.
- The comments on `JsonAddressSpaceEntry::selectedBy` and `Channel::selectedBy` in
  `src/JsonMapFileParser.cc` that call a register-level selector next to `channels`
  an authoring error, and a channel selector an override, go with the rules above.

### JSON schema (modified)

Location: schemas/jmap.schema.json (modified)

- The schema states the rules for external validators; the parser stays the
  enforcement point, and no test validates the schema, so a drift between the two is
  caught by review only.
- `addressSpaceEntry`: the register and module classification of `jmap_modules`
  above, as description only, since a per-entry schema cannot see whether an address
  or a void entry sits above the entry.
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
  rejection of an own `width` on it.
- `access` and `triggeredByInterrupt`: state the inheritance and the exclusion of the
  pair.
- `channel` and `channelChild`: a `void` `representation` is rejected, since both
  need a width, and a channel requires `offset`. The channel-child definition keeps
  only `representation`, so `additionalProperties: false` rejects every other member
  except the `^_` comments, and its description states that the channel supplies the
  address, the element size and the selector.
- The void-typed entry: the description says that it forbids an own `address` and
  inherits none, so the address chain below it starts afresh, the latter being no
  constraint a per-entry schema can express; `doubleBuffering`, `channels`, the
  geometry members and an own `address` are rejected as new constraints in the
  `allOf` block, and `triggeredByInterrupt` is required.
- `doubleBuffering` gets a `required` list naming `secondaryBufferAddress`, which the
  definition does not have today. `secondaryBufferAddress` gets its own definition
  instead of the `$ref` to `#/definitions/address`: it carries only the required
  `offset` and rejects `type`/`channel` through `additionalProperties`.

### JsonMapFileParser::parse (modified)

Location: src/JsonMapFileParser.cc (modified)

- Rejects the maps that the validity rules of the requirements exclude, without a
  format version bump: its version check stays untouched, while `validate()` owns
  the individual rejections specified above.
- Calls `addInfos` for every top-level entry straight after parsing. That loop and
  the recursion in `addInfos` bind their entries by `auto&` instead of `const auto&`,
  since `addInfos` records the parent link and the path of the sub-entries.

### chimeratk-map-to-jmap (modified)

Location: tools/chimeratk-map-to-jmap (modified)

- Writes `type` and `access` on every register, because a converted map would resolve
  them differently below a `DMA`, `RO` or interrupt-bearing parent. The `INTERRUPT`
  guard stays, so an interrupt register keeps writing `triggeredByInterrupt` and no
  `access`, which `validate()` rejects as a pair. The three places that emit `access`
  collapse into one step at the end of `_fill_entry_from_reg`, so no branch can omit
  the value.
- Omits a geometry value it cannot supply instead of writing `0`, and writes an
  explicit `representation`, so no register changes its word by an omitted `width`.
- Drops `_ensure_void_parents` and the unused `parent_data` parameter: a top-level
  void-typed entry parses without an address-bearing parent, and the synthetic one
  would become a register under the new rules.

### chimeratk-device-to-jmap (not modified)

Location: tools/src/chimeratk-device-to-jmap.cc

- It keeps emitting `{"type": "string"}` without a `width`, which now resolves to
  `8` on the parser's side, so it needs no change.

- Making the width of a `string` `representation` follow the element size like a
  fixed-point one: rejected. A character is 8 bits, and `bytesPerElement: 40` would
  otherwise turn a string register into a 320-bit word.
- Making the two generators emit an explicit width instead of giving `string` a fixed
  one: rejected. It would force both tools to restate a rule of the format.

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
- Reusing `RepresentationType` instead of the parser's own enum: rejected for this
  change. The namespace of the catalogue type differs and the conversion would move
  into the getters, which R7 keeps free of catalogue concerns.

## Test plan

All parser cases go into `tests/executables_src/testJsonMapFileParser.cpp`. Each
loads `tests/simpleJsonFile.jmap`, or the fixture it names, and applies its mutation
through one shared helper that injects it, writes a temporary file and parses it. The
helper is the generalised form of `parseInjectedInterruptMap`: it takes the base
document as well as the mutation, so it serves the whole-document cases that
`parseWithMapFormatVersion` and the selectedBy fault helpers build today, and they
fold into it. No fault-only fixture is added.

The fixture is shared with the backend test binaries and with
`TestAllTwoDimensionalSkipped` of `testDeviceToJmap.cc`, which reads
`tests/muxedDataAccessor.jmap`; no case affects what those see. A case relying on an
existing entry names it, so a fixture edit that changes what it inherits shows up in
review. Each case's `Requirement:` line below names the `R` bullets it covers, and its
`Necessary:` line states why no other case covers them; neither appears in the test
source. A rejection case expects
`ChimeraTK::logic_error` and nothing more; each rejection has its own case, except
where two mutations restate one rule over the same member.
mutation and one parse per rule.

- `tests/executables_src/testJsonMapFileParser.cpp` (modified)
  - `TestSelectedByInheritance` (modified): after `INHERIT.REG_B`,
    `INHERIT.REG2D_OWN` and `INHERIT.CHANB.D1` lose their own selectors, every
    register and channel below `INHERIT` expects the selector `INHERIT` declares
    (`/APP/OUTPUT_SELECT`, 3), and the channel `D0`/`D1` pair of `CHANB` expects it
    twice.
    - Requirement: R8 (`selectedBy` inheritance and no override).
    - Necessary: the case is the only one with a module-level declaration, a nested
      module (`SUB`) and 2D channels, so it keeps covering inheritance after the
      three overrides go; a register declaring a selector itself is covered by
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
  - `TestGoodMapFileParse` (modified): its assertions move to the resolved values
    that the change produces, among them `BSP.SOME_INFO`, whose `representation`
    omits `width`, so its word becomes the `8` bits of a string character.
    - Requirement: R7, R14.
    - Necessary: it is the only case that walks the whole shared fixture, so it is
      where a fixture entry whose resolution changed shows up.
  - `TestChannelsEntryInheritsAddressBecomesRegister`: an entry with `channels` and
    a `pitch`, neither an own `address` nor a `representation`, below an
    address-bearing entry on `DMA` channel 3 at a non-zero offset with
    `numberOfElements: 2`; it yields a register on that address with one element and
    its `pitch * 8` as the pitch, its channel slices resolve to that address and
    carry the channel's `bytesPerElement`, and a bit-field child of one channel, say
    `bitShift: 8` and `width: 4`, resolves there too with a slice of its own. A
    second channel omits its `bytesPerElement` and defaults to `4`; a third omits
    its `representation` and gets a fixed-point word of its element size.
    - Requirement: R5 (a 2D register and its channels), R4 (no geometry inheritance
      with `channels`).
    - Necessary: every `channels` entry in the existing maps carries its own
      `address`, so an address-less register passing its address to a channel and on
      to a bit field is untested.
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
      entry below an address that gives only `children`, and none that gives neither
      of the three.
  - `TestModuleWithAccessRejected`, `TestModuleWithTriggeredByInterruptRejected`,
    `TestModuleWithNumberOfElementsRejected`,
    `TestModuleWithBytesPerElementRejected`,
    `TestModuleWithDoubleBufferingRejected`, `TestModuleWithPitchRejected`: each
    gives one data member on a true module (one with no address in its ancestry) and
    expects the rejection; a module carrying only `engineeringUnit` and `description`
    next to `children` parses.
    - Requirement: R11.
    - Necessary: `MY_INTC` in the fixture is a true module, and no fixture module
      carries any of the members; the parsing module shows the allowed members stay
      allowed.
  - `TestGeometryInheritanceAndOwnSampleWidth`: an address-bearing parent with
    `numberOfElements: 2` and `bytesPerElement: 8` and five children: the first
    three are address-less and carry a `representation` - the first inherits both
    values, the second gives `bytesPerElement: 2` and keeps the inherited `2`
    elements, the third gives `numberOfElements: 3`, which supersedes the inherited
    `2` - the fourth gives a `representation` with only its `type`, so its `width`
    defaults to the element size in bits, and the fifth gives an own `address` with
    only its `offset`, which stops the geometry inheritance, so it keeps one element
    of four bytes. In each case the own `bytesPerElement` shows in
    `elementPitchBits`, so the inherited and the defaulted value stay apart.
    - Requirement: R4, R6, R7 (an omitted `width`).
    - Necessary: no case overrides one of the two inherited geometry values below a
      parent with a non-default geometry, and no case gives a `representation`
      without `width`; `BSP.VERSION` and `BSP.SOME_INFO` inherit from `BSP`, but only
      a child that overrides one value shows the inherited other one.
  - `TestChannelsWithOwnRepresentationRejected`: a 2D register that also gives a
    `representation` is rejected.
    - Requirement: R5 (`channels` and `representation` are mutually exclusive).
    - Necessary: no fixture gives both, and the parser ignored the `representation`.
  - `TestVoidChannelRejected`: a channel of a 2D register whose `representation` is
    `void` is rejected.
    - Requirement: R5.
    - Necessary: no fixture has a void channel, and `Channel::fill` would emplace a
      VOID channel with a width instead of rejecting it.
  - `TestVoidChannelChildRejected`: a bit-field child of a channel with a void
    `representation` is rejected.
    - Requirement: R5.
    - Necessary: the bit-field rule of R5 is separate from the channel rule, and no
      fixture has a void bit field.
  - `TestTwoDimensionalEntryWithChildrenRejected`: a 2D register that also gives
    `children` is rejected.
    - Requirement: R5.
    - Necessary: no fixture gives both, and the parser iterated the `children`.
  - `TestChannelChildWithoutBitShiftRejected`: a bit field without `bitShift` is
    rejected.
    - Requirement: R5, R13.
    - Necessary: the parser defaulted a missing `bitShift` to `0`, so the case
      reached no rejection before.
  - `TestStringWidthRejected`: a `string` `representation` that gives a `width` is
    rejected.
    - Requirement: R7, R13.
    - Necessary: a string's width is fixed at `8`, and a map that states another one
      must not be overridden silently.
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
  - `TestSecondaryBufferAddressWithoutOffsetRejected`: a present
    `secondaryBufferAddress` without `offset` is rejected.
    - Requirement: R2.
    - Necessary: no existing map has a secondary address without `offset`.
  - `TestDoubleBufferingWithoutSecondaryAddressRejected`: a `doubleBuffering` object
    that omits `secondaryBufferAddress` is rejected.
    - Requirement: R2.
    - Necessary: every existing `doubleBuffering` gives it, so omitting it parses
      today.
  - `TestSecondaryBufferAddressWithTypeRejected`: a `secondaryBufferAddress` that
    gives its own `type: DMA` next to the required `offset` is rejected.
    - Requirement: R2.
    - Necessary: every existing map gives a `type` there; without the rejection the
      value would silently have to match the primary channel.
  - `TestSecondaryBufferAddressWithChannelRejected`: a `secondaryBufferAddress` that
    gives a `channel` is rejected.
    - Requirement: R2.
    - Necessary: the channel half of the rule is separate, and no fixture gives one.
  - `TestAccessInheritanceWithOwnAddress`: an address-bearing parent with `access:
    RO`, a `representation` of its own (`IEEE754`, `width: 64`), `engineeringUnit`
    and `description` and two children whose own `address` carries only `offset`;
    both are `READ_ONLY`. The first gives its own `representation` and is not
    reinterpreted by the parent's; the second omits it and gets the defaulted
    fixed-point word, not the parent's. The two children leave `engineeringUnit` and
    `description` empty, so these fields are not inherited.
    - Requirement: R3, R9.
    - Necessary: no fixture combines an own `address` with an omitted `access` below
      a parent whose access is not the default.
  - `TestTriggeredByInterruptInheritance`: an address-bearing parent with
    `triggeredByInterrupt` and five children; the address-less one without `access`
    is an `INTERRUPT` register with the parent's interrupt id, the address-less one
    giving `access: RO` is `READ_ONLY` without an interrupt id, the one whose own
    `address` gives only `offset` is `INTERRUPT` as well, a void-typed child with its
    own `triggeredByInterrupt` keeps that id instead of the parent's, and a child
    below a parent with `access: RO` that gives its own `triggeredByInterrupt` is
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
  - `TestZeroNumberOfElementsRejected`: a `numberOfElements: 0` on an otherwise valid
    register is rejected.
    - Requirement: R4.
    - Necessary: `0` means "not set" today, so the new reading needs its own case.
  - `TestZeroBytesPerElementRejected`: a `bytesPerElement: 0` on an otherwise valid
    register is rejected.
    - Requirement: R4.
    - Necessary: the register half of the rule, separate from the channel half.
  - `TestZeroChannelBytesPerElementRejected`: a channel's `bytesPerElement: 0` is
    rejected.
    - Requirement: R4.
    - Necessary: R4 covers a channel explicitly, and no fixture gives `0` there.
  - `TestZeroPitchRejected`: a `pitch: 0` on a 2D register is rejected.
    - Requirement: R4.
    - Necessary: the pitch half of the rule.
  - `TestDoubleBufferingNotInherited`: a register below a double-buffered parent and
    without a `doubleBuffering` of its own is not double-buffered.
    - Requirement: R9.
    - Necessary: nothing checks that `doubleBuffering` stays off a child, and the
      parser reads only the own member.
  - `TestDoubleBufferingWithInheritedAddress`: a `doubleBuffering` register without
    an own `address`, below an address-bearing entry on `DMA` channel 3 at a non-zero
    offset, gets its `BUF0` view on the resolved address and its `BUF1` view on the
    address from `secondaryBufferAddress`, both on the same channel.
    - Requirement: R2, R1.
    - Necessary: every existing `doubleBuffering` gives its own `address`, so an
      inherited one with its views is untested.
  - `TestSelectedByOverrideRejected`: an entry below a `selectedBy`-declaring parent
    that gives its own `selectedBy` is rejected; so is the channel `INHERIT.CHANB.D1`
    mutated back to declaring one.
    - Requirement: R8 (never overridden).
    - Necessary: the fixture declares `selectedBy` on one entry only, so an own value
      below a declarer parses today, and the channel half of the rule needs its own
      mutation.
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
    - Requirement: R10, R13.
    - Necessary: every fixture VOID entry has none, and the parser took the address.
  - `TestVoidEntryWithoutInterruptRejected`: a VOID-typed entry without its own
    `triggeredByInterrupt` is rejected.
    - Requirement: R10, R13.
    - Necessary: R10 makes the member mandatory, and no case reaches the rejection.
  - `TestVoidEntryGeometryRejected`: VOID-typed entries giving an own
    `numberOfElements`, `bytesPerElement` and `pitch` are rejected, each as its own
    mutation.
    - Requirement: R10.
    - Necessary: no fixture VOID entry gives any of the three.
  - `TestVoidEntrySelectedByRejected`: a VOID-typed entry that gives an own
    `selectedBy` is rejected.
    - Requirement: R10.
    - Necessary: the selectedBy half of the void rule, separate from the geometry.
  - `TestVoidEntryTakesNoAddress`: a VOID-typed entry below an address-bearing parent
    on `DMA` channel 3 at a non-zero offset is a register with `address` 0 and
    `nElements` 0.
    - Requirement: R10 (no inherited address).
    - Necessary: the VOID entries of the fixture (`BSP.VOID_INTERRUPT_0` and its
      sibling `BSP.VOID_INTERRUPT_3_0_1`) sit below an address-bearing entry and only
      their `pathName` is asserted today.
  - `TestVoidEntryChildren`: one child below the VOID entry that gives a
    `representation` but no own `address` is rejected, since the VOID entry passes
    none on; a sibling whose `address` gives only `offset` and which gives no
    `access` parses as an `INTERRUPT` register with the VOID entry's id, resolved to
    `IO` and channel `0`, so it took the `IO` default rather than a value from above;
    a `selectedBy` declared above the VOID entry reaches it.
    - Requirement: R10, R1, R8.
    - Necessary: no fixture has children below a VOID entry, so neither the
      rejection nor the default behind a VOID entry is reached elsewhere.
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
- Existing cases adapted, not extended:
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
  - Its round trips keep passing, since the device-to-jmap tool emits the same maps.

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
- DI-6 [FOLDED-IN] The bit-field rejection repeated the missing-`bitShift` case and
  left the effective width of an omitted `width` undefined; folded in as the ignore
  rule of CR-003, with `width` optional and `bitShift` required.
- DI-7 [FOLDED-IN] `makeChannelInfo` needs the resolved width, which the plan's
  signature does not carry.
- DI-8 [ACCEPTED] CR-016 (`mhier-agent/CR-016-runtime-selectedby-enforcement`)
  renames `selectedBy` to `gatedBy`, records the 2D register-level gate as
  `registerGatedBy` and states that it is not inherited by channel slices. It must
  therefore depend on this change and be rebased onto the `selectedBy` rules here;
  the `Depends on: CR-018` line belongs into CR-016, not into this document.
