# CR-020: Register-name references in triggeredByInterrupt

Synopsis: A register may state its interrupt id as the name of another
interrupt register instead of a numeric id array. The referring register
takes the resolved id of that register, declared explicitly or derived
implicitly.

Depends on: CR-019

Status: DONE

## Requirements

Aspect: interrupt ids referred to by register name.

- The `triggeredByInterrupt` field of an address-space entry may be a string
  naming another register, as an alternative to the numeric id array.
- The named target must be an interrupt register: a catalogue register with
  `Access::INTERRUPT`, whose id it declares itself or, for a derived `ISR` bit
  field, derives implicitly (CR-019 adds these implicit ids).
- The target may carry data; triggering does not depend on the target's data,
  so a non-void target is allowed.
- The referring register takes the target's resolved interrupt id and is
  otherwise declared as today (own address, representation and data).
- The referring register may be void or data-carrying. It is read-only with
  `wait_for_new_data`, exactly as for a numeric id.
- The reference resolves transitively: a target may itself name another
  register.
- A target may be a derived `ISR` child and may be declared anywhere in the
  file.
- An error while parsing the map file for an empty interrupt declaration:
  - A present but empty `triggeredByInterrupt` array.
  - An empty reference string.
- After parsing, the interrupt bookkeeping of the map file is complete: every
  interrupt register it declares is listed by `getListOfInterrupts()` and
  resolves through its canonical interrupt path, the `!<id>` path.
- A string reference and the equivalent numeric array are interchangeable, and
  the referring register behaves exactly like one declared with the resolved
  id.
- Registers resolving to the same id are notified together and share the
  canonical interrupt path, as for a written id.
- The field stays mutually exclusive with `access` and remains required for a
  void register.
- An error while parsing the map file for a reference that does not resolve:
  - The target does not exist.
  - The target is not an interrupt register.
  - The reference is cyclic, including a register naming itself.
- Only the JSON (`*.jmap`) parser accepts a register-name reference; the
  `*.mapp` parser keeps numeric ids and its behaviour is unchanged.
- Hidden registers are out of scope: a map file cannot declare one, and no
  rule of this change applies to them. It is an accepted limitation that
  `triggeredByInterrupt` on a hidden register which names another register
  is not resolved by the parser.

## Specifications

### doc/jmapFormat.dox (modified)

- The `jmap_interrupts` section states that `triggeredByInterrupt` may name
  another interrupt register instead of giving the numeric id array.
- It states that the reference is followed transitively, so a target may itself
  name a register.
- It states that a cycle is an error.
- It states that the target must be an interrupt register.
- It states that a name which does not exist, or which does not name an
  interrupt register, is an error while parsing the map file.

### schemas/jmap.schema.json (modified)

- `triggeredByInterrupt` accepts a non-empty string beside the non-empty
  numeric array.
- The `interruptHandler` conditional for a chained interrupt is restricted to
  `type: array`. Without this, `minItems` is ignored for a string and every
  string reference would demand a handler entry; the target's own numeric
  chain already triggers the requirement where one is needed.

### NumericAddressedRegisterInfo (modified)

Location: include/NumericAddressedRegisterCatalogue.h (modified),
src/NumericAddressedRegisterCatalogue.cc (modified)

- `InterruptId` (new): `using InterruptId = std::variant<std::vector<size_t>,
  std::string>;`, a public alias naming the two forms of an interrupt id, so
  the field and the constructor parameters name it instead of spelling the
  variant out. It is part of the installed header's API; the spelled-out type
  and the alias are the same type.

- `interruptId` (modified): of type `std::optional<InterruptId>`, replacing the
  plain `std::vector<size_t>`. Field and constructor parameters are spelled
  through the alias, so downstream code constructs the field through
  `NumericAddressedRegisterInfo::InterruptId` too.
  - The variant carries the numeric id or a not yet resolved register-name
    reference; `std::nullopt` is a register that is not an interrupt source.
  - The JSON entry field has the same type, so the parser assigns it unchanged.
  - The JSON parser rejects a present empty `[]`, and the `*.mapp` parser
    passes `std::nullopt` for a register without interrupt. A vector otherwise
    reaches the field only from a caller that passes one, and the finalisation
    rejects an empty one, so the member documentation stops promising that the
    numeric alternative is never empty.
- Constructors (modified): both take `std::optional<InterruptId>`, with `=
  std::nullopt` as the default of both of them.
  - The default stays on both: the 1D constructor is the default constructor of
    the type, which the catalogue needs as the mapped type of its register map,
    and in-tree callers rely on the trailing arguments being defaulted.
  - Both constructors store the id as given, so a register is an interrupt
    source exactly when it was constructed with an id.

- `numericInterruptId` (new): `[[nodiscard]] std::optional<std::vector<size_t>>
  numericInterruptId() const`.
  - The numeric id, or `std::nullopt` when the field is absent or holds a
    reference. Follows the value-returning style of the other optional members
    and avoids handing out a pointer into the object.
  - A `std::vector<size_t>` assigns into the field directly (it converts to the
    numeric alternative), so no setter is needed.

- `getQualifiedAsyncId` (modified): returns
  `numericInterruptId().value_or(std::vector<size_t>{})`, an empty vector when
  the field is absent, as today. The empty vector is named explicitly because
  `std::optional::value_or` takes a deduced forwarding reference and cannot
  deduce it from a braced-empty argument.
  - No throw of its own: the finalisation resolves or rejects a leftover
    reference, so letting it return an empty vector keeps the accessor
    non-throwing.

- The header gains three standard includes:
  - `<variant>` for the alias.
  - `<optional>` for the optional members, which were reached only transitively
    before.
  - `<vector>` because the alias names it.

- The member is public and the header installed, so the type change breaks
  source and binary compatibility for readers, in tree and downstream.

- In tree the JSON parser, the interrupt distributors and the tests are
  updated; downstream code migrates to `numericInterruptId()`.

### NumericAddressedRegisterCatalogue (modified)

Location: include/NumericAddressedRegisterCatalogue.h (modified),
src/NumericAddressedRegisterCatalogue.cc (modified)

- `addRegister` and `modifyRegister` (removed): the base class already declares
  both signatures, so the overrides and their bookkeeping go.
  - Their doxygen blocks go with them, including the promise that
    `modifyRegister` updates the interrupt bookkeeping.
  - No register is ever in the interrupt indexes while its id is missing or
    still a reference.

- `addInterruptBookkeeping` (removed): its body moves into
  `finaliseInterruptBookkeeping`.

- `finaliseInterruptBookkeeping` (new): `void finaliseInterruptBookkeeping()`.
  - Public. Must be called after the caller's last change to the register set,
    i.e. after its last `addRegister`, `modifyRegister` or `removeRegister`; a
    further call is harmless. The obligation is stated in three places, see the
    class documentation, the method documentation and
    `NumericAddressedBackend`.
  - Resolves the references first, so no register is indexed while its id is
    still a name: it collects the paths of the registers whose `interruptId`
    holds a string and writes each resolved id back with `modifyRegister`. The
    collection keeps the catalogue unmodified while it is iterated.
  - Then fills `_listOfInterrupts` and `_canonicalInterrupts` from the
    registers with `Access::INTERRUPT`.
  - Both passes walk the registers the catalogue presents. Hidden registers are
    out of scope (Requirements), so nothing is done for them: they are neither
    indexed nor resolved.
  - Throws a `ChimeraTK::logic_error` naming the register when a walked
    `Access::INTERRUPT` register carries no id or an empty one. Neither the
    field nor the constructors rule this state out, and neither the catalogue
    nor its register info belongs to the backend-facing API, so the throw
    guards misuse rather than checking input. An unresolved reference never
    reaches this point: the resolution pass settles or rejects every reference
    of a walked register.
  - Rebuilds both indexes from scratch, so no completion flag is needed.
  - The catalogue owns both the unresolved state and its resolution, so a
    catalogue built through the public constructors holds a state the class
    itself can settle.

- `resolveInterruptReference` (new, private): `std::vector<size_t>
  resolveInterruptReference(const RegisterPath& targetPath) const`.
  - Returns the numeric id of the register the given path names.
  - The target is read through the catalogue, since the field carries the
    target name.
  - Follows the chain of named targets while keeping the already visited target
    paths, so it continues into a target that itself holds a reference. A path
    that repeats throws a `logic_error`, so a self-reference or a cycle is an
    error instead of a recursion without end.
  - A target must carry `Access::INTERRUPT`; a register whose id is a name but
    which lacks that access is not a valid target.
  - The name held by the referring register is converted to a `RegisterPath`
    with the dot alt separator by the caller, and every further name along the
    chain by this helper, so a dot-separated target resolves as for
    `selectedBy`.
  - A missing target, a target that is not an interrupt register, and a
    repeated path throw a `logic_error` naming the registers involved.

- `getBackendRegister` (modified): keeps synthesising canonical interrupt path
  entries, which exist only after finalisation, and its error for an unknown
  `!` path states that a missing `finaliseInterruptBookkeeping()` is a possible
  cause. Without that, a missed call reports a legal path as an illegal one.
  No test asserts the message.

- Class documentation (new): a doc block on the class states that a filled
  catalogue must be finalised once with `finaliseInterruptBookkeeping()` before
  it is used, and that it applies to whoever fills a catalogue, the parsers of
  this project being its in-tree callers; the class has no doc block
  today.

### NumericAddressedBackend (modified)

Location: include/NumericAddressedBackend.h (modified),
src/NumericAddressedBackend.cc (modified)

- Constructor (modified): gains documentation.
  - `NumericAddressedBackend(const std::string& mapFileName,`
    `std::unique_ptr<NumericAddressedRegisterCatalogue> registerMapPointer,`
    `const std::string& dataConsistencyKeyDescriptor)`
  - It states that a catalogue parsed from the map file name is finalised by
    the parser and needs no further call, and that the backend never finalises
    a passed catalogue itself: a catalogue it takes over is empty or already
    finalised. The constructor is undocumented today.

- `getRegisterAccessor_impl` (modified): asserts that the qualified async id of
  the interrupt register is not empty before the accessor is built.
  - `src/NumericAddressedBackend.cc` takes `.front()` of
    `getQualifiedAsyncId()`, which is empty for an interrupt register without an
    id in a catalogue that was never finalised. The guard turns that undefined
    behaviour into an assertion failure.
  - It guards a programming error, not user input: both parsers finalise the
    catalogues they produce and a backend only takes over an empty or already
    finalised one, so only hand-built catalogue code reaches it. An assertion
    cannot be tested, so the Test plan covers it nowhere.

### detail::JsonAddressSpaceEntry (modified)

Location: src/JsonMapFileParser.cc (modified)

- `triggeredByInterrupt` (modified):
  `std::optional<NumericAddressedRegisterInfo::InterruptId>`, replacing the
  plain id vector, so an entry carries the numeric id array or a register name
  and `fill` assigns it to `info.interruptId` unchanged. An entry declares an
  interrupt when the field holds a value; both declaration sites in `fill` test
  `!triggeredByInterrupt.has_value()`.

- `fill` (modified): on a declaring entry it assigns `info.interruptId` from
  `triggeredByInterrupt` as a whole, for the data and the void branch alike.
  - A present empty array is a `logic_error` naming the entry: it declares no
    interrupt and has no meaning, and the parser does not enforce the schema's
    `minItems`. It is rejected before the declaration is acted on, so it does
    not count as no declaration.
  - An empty reference string is a `logic_error` naming the entry: it names no
    register, and the schema's `minLength` is likewise not enforced.
  - The reference string is stored as written; the dot alt separator is applied
    where the reference is resolved.

### detail::JsonMapFileParser::Imp (modified)

Location: src/JsonMapFileParser.cc (modified)

- `parse` (modified): after `InterruptHandlerEntry::augmentCatalogue` and
  before the `selectedBy` validation loop, it calls
  `finaliseInterruptBookkeeping()`, which resolves the references and builds
  the interrupt indexes.
  - The parser performs no resolution of its own: `finaliseInterruptBookkeeping()`
    owns it.
  - Runs after the augmentation, so the implicit `ISR` ids exist and a derived
    register can be a target.
  - Runs before the `selectedBy` validation and the metadata handling, so every
    interrupt id and canonical path is final before either walks the catalogue.

### TraditionalMapFileParser (modified)

Location: include/TraditionalMapFileParser.h (modified),
src/TraditionalMapFileParser.cc (modified)

- `parse` (modified): calls `finaliseInterruptBookkeeping()` on the parsed
  catalogue before returning it.
  - The `*.mapp` parser writes numeric ids only, so nothing needs resolving,
    but the catalogue now requires the explicit finalisation step.
- Register information (modified): a register without interrupt is built with
  `std::nullopt` instead of an empty id vector.
  - The parser already sets `Access::INTERRUPT` from a non-empty id vector, so
    it passes `std::nullopt` for the empty case and its behaviour is unchanged.
  - That keeps an empty numeric id out of every catalogue, so no producer
    normalises one.

### nlohmann::adl_serializer (modified)

Location: include/JsonExtensions.h (modified)

- `adl_serializer<std::optional<T>>` (unmodified in behaviour): a null value
  and a missing field give `std::nullopt`, and `std::nullopt` is written as
  null.
  - `WITH_DEFAULT` assigns the default-constructed member for a missing key, so
    the absent case rests on that default, not on the member being left
    untouched. This is why `doubleBuffering` works today.

- `adl_serializer<std::variant<Ts...>>` (new): mirrors that support for the new
  field.
  - `to_json` writes the active alternative; a variant always holds a value, so
    nothing is written as null.
  - `from_json` takes the first alternative that parses:
    - The alternatives are tried in declaration order.
    - A null is skipped explicitly.
    - An error is reported when no alternative matches.
  - The variant specialisation is generic, so it applies to every
    `std::variant` serialised in a translation unit including this header; the
    `triggeredByInterrupt` field is currently the only such use.
  - Composed on the field: absent or null gives `std::nullopt`, an array the id
    vector, a string the register name. With both mappings the
    `NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT` list stays unchanged.
  - `src/JsonMapFileParser.cc` is the only file including the header, so the
    specialisation reaches no other translation unit in tree; downstream it
    applies only where the header is included explicitly.

### async::GenericMuxedInterruptDistributor (modified)

Location: include/async/GenericMuxedInterruptDistributor.h (modified),
src/async/GenericMuxedInterruptDistributor.cc (modified)

- `augmentCatalogue` (modified): builds the child id as a local
  `std::vector<size_t>` from the controller id plus the bit position and
  assigns it to the field, which converts to the numeric alternative. Replaces
  the append onto the member, which a variant does not offer.
  - It runs before the finalisation, so the registers it turns into interrupt
    registers are indexed there rather than at the call.
  - Its documentation states that the interrupt indexes are built only by
    `finaliseInterruptBookkeeping()`, so an augmenter run on an already
    finalised catalogue leaves them stale.

### Alternatives considered

- A side map of pending references filled while `addInfos` runs: rejected. It
  duplicates the field and needs one recording point per kind of derived
  register, so every new kind would have to be taught that recording.

- A guard against an empty id in the per-register bookkeeping that this change
  removes, i.e. in the `addRegister` and `modifyRegister` overrides: rejected.
  The finalisation checks the empty-id case once for the whole catalogue and
  throws for it, so a guard per mutation would be dead code.

- Resolving the references in the parser: rejected. The unresolved field stays
  readable by code that cannot settle it, and the state would be split between
  parser and catalogue.

- A specialisation of `adl_serializer` for the concrete variant type: rejected,
  would tie the mapping to this one field, whereas the generic
  `adl_serializer<std::variant<Ts...>>` mirrors the existing `std::optional`
  support.

- A dedicated struct wrapping the two forms (the `SelectedBy` idiom): rejected.
  `SelectedBy` pairs a path with a value and is all-or-nothing, whereas
  `triggeredByInterrupt` has three states (absent, id array, name);
  `std::optional<std::variant<...>>` expresses that directly.

- A string element inside the id array (e.g. `[3, 0, "REG"]`): rejected. A name
  yields a whole id, so mixing it with numeric components has no meaning.

- Resolving when the interrupt distributor is created (runtime): rejected. The
  id must exist at parse time, as in CR-019.

- Requiring the target to be void: rejected. The trigger does not depend on the
  target's data.

- Resolving before the augmentation: rejected. Implicit `ISR` ids are known
  only after it.

- A `finalised` flag guarding the indexes: rejected. It only turns a missed
  call into a different error message; the obligation is documented on the
  method, on the class and on the catalogue-taking backend constructor, and the
  parsers that produce complete catalogues are tested.

## Test plan

- `tests/executables_src/testJsonMapFileParser.cpp` (modified)
  - Bring the existing `interruptId` assertions in line with the new field
    type: those that compare the field against a bare `std::vector<size_t>` now
    compare it against a `std::variant<std::vector<size_t>, std::string>`
    holding the id. The two assertions that compare with
    `boost::test_tools::per_element()` keep the element-wise comparison by
    reading the id through
    `numericInterruptId().value_or(std::vector<size_t>{})` instead, which the
    variant itself does not offer. An assertion spells the variant type out;
    the shorthand construction from a braced list does not compile inside
    `BOOST_TEST`, because its decomposition cannot form the overload.
    Mechanical, but it touches most interrupt assertions in the file.
  - `TestInterruptReferenceVoidReferrer`: a `type: void` register declaring its
    `triggeredByInterrupt` as a string carries the target's resolved id, and the
    parse succeeds, which an unresolved reference would not.
    - Covers the void referrer requirement and the assignment in `fill`; a
      void register has no address, so this is the declaration path a string
      reference has to work in.
    - The referrer shares its target's id, so its entry in
      `getListOfInterrupts()` and its canonical interrupt path coincide with the
      target's and cannot be told apart from it; the assertion on the referrer's
      own `interruptId`, together with the parse succeeding, is what the case
      contributes.
  - `TestInterruptReferenceToDataRegister`: the same assertions, with a target
    that carries data; its own bookkeeping entry and canonical path again
    coincide with the target's for the reason above.
    - Covers the string form and the resulting bookkeeping (Requirements; the
      `NumericAddressedRegisterInfo` and catalogue specifications), and the
      requirement that a target may carry data. Its assertions are the same as
      those of the numeric-id cases, so the string form is interchangeable with
      the resolved id array (Requirements).
  - `TestInterruptReferenceToDerivedIsrChild`: the reference names a derived
    `ISR` bit-field child and resolves after the augmentation.
    - Covers the resolution ordering (Requirements; `Imp::parse`).
  - `TestInterruptReferenceChain`: two references resolve to the final id,
    dot-separated targets included, and a target is declared after the register
    that names it.
    - Covers the transitive requirement and that a target may follow the
      referring register in the file, both resting on the post-pass resolution.
  - `TestInterruptReferenceMissingTarget`: the reference names no register, a
    parse-time `logic_error`.
    - Covers the rejection requirement for a name that does not exist, i.e.
      that the failure surfaces as a map-file error instead of escaping the
      parser.
  - `TestInterruptReferenceNonInterruptTarget`: the reference names a register
    that is not an interrupt register, a parse-time `logic_error`.
    - Covers the rejection requirement for a target without
      `Access::INTERRUPT`.
  - `TestInterruptReferenceCycle`: two registers name each other, a parse-time
    `logic_error`.
    - Covers the cycle requirement.
  - `TestInterruptReferenceSelf`: a register names itself, a parse-time
    `logic_error`.
    - Covers the Requirement that a self-reference is a cycle like any other.
  - `TestInterruptReferenceEmptyArray`: a present empty `triggeredByInterrupt`
    array, a parse-time `logic_error`.
    - Covers the empty-array requirement; the schema already forbids it, but
      the parser does not validate against the schema.
  - `TestInterruptReferenceEmptyString`: an empty reference string, a
    parse-time `logic_error` naming the entry.
    - Covers the empty-name requirement, likewise outside the schema check.
- `tests/executables_src/testGenericMuxedInterruptDistributor.cpp` (modified)
  - `testInterruptReferenceEndToEnd`: the only end-to-end case. It writes a
    copy of the existing fixture map to a temporary file, with a data register
    added that names a derived `ISR` bit-field child of that map, and parses the
    copy, so no fixture file is added and no other test's expectations change.
    - Both the referring register and the child it names are notified on the
      same `ISR` bit, so registers sharing the resolved id are notified
      together.
    - A non-matching `ISR` bit does not notify it, so the resolved id reaches
      the distributor.
    - Covers the runtime side of the Requirements once; this single run also
      exercises augmentation, resolution and finalisation together.
- `tests/executables_src/testMapFileParser.cpp` (modified)
  - `testInterruptBookkeepingFinalised`: the `*.mapp` parser still yields every
    interrupt register in `getListOfInterrupts()` and through its canonical
    interrupt path.
    - Covers the finalisation call in `TraditionalMapFileParser::parse`, which
      no other test covers.
    - The expected register infos that the file compares have no interrupt id
      and keep their empty braced argument `{}`, which the `std::optional`
      parameter takes as `std::nullopt`, so the compared values are unchanged.
      A construction that passes an interrupt id spells the vector out as
      `std::vector<size_t>{...}`, because a braced list such as `{3}` does not
      form the variant inside the optional. The pre-existing assertions read the
      id only through `operator==` of the register info and stay unchanged; the
      `printRegInfo` debug helper reads it through
      `numericInterruptId().value_or(std::vector<size_t>{})`, naming the empty
      vector explicitly as `getQualifiedAsyncId` does.
