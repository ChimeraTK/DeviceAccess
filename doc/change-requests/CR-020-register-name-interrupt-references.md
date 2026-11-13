# CR-020: Register-name references in triggeredByInterrupt

Synopsis: A register may state its interrupt id as the name of another
interrupt register instead of a numeric id array. The referring register
takes the resolved id of that register, declared explicitly or derived
implicitly.

Depends on: CR-019

Status: READY TO IMPLEMENT (from TESTS REVIEWED)

## Requirements

Aspect: interrupt ids referred to by register name.

- The `triggeredByInterrupt` field of an address-space entry may be a string
naming another register, as an alternative to the numeric id array.
- The named target must be an interrupt register: one carrying a
`triggeredByInterrupt` of its own, declared explicitly or derived
implicitly (CR-019).
- The target may carry data; triggering does not depend on the target's data,
so a non-void target is allowed.
- The referring register takes the target's resolved interrupt id and is
otherwise declared as today (own address, representation and data).
- The referring register may be void or data-carrying. It is read-only with
`wait_for_new_data`, exactly as for a numeric id.
- The reference resolves transitively: a target may itself name another
register.
- A cycle, including a register naming itself, is an error.
- References resolve after CR-019 has augmented the implicit interrupt ids,
so a target may be a derived `ISR` child and may be declared anywhere in
the file.
- A present but empty `triggeredByInterrupt` array, and an empty reference
string, are errors while parsing the map file.
- After parsing, the interrupt bookkeeping is complete: every interrupt
register is listed by `getListOfInterrupts()` and its canonical interrupt
paths resolve.
- A string reference and the equivalent numeric array are interchangeable,
and the referring register behaves exactly like one declared with the
resolved id.
- Registers resolving to the same id are notified together and share the
canonical interrupt path, as CR-019 already allows for a written id.
- The field stays mutually exclusive with `access` and remains required for a
void register.
- An error while parsing the map file when the target does not exist, when
the target is not an interrupt register, or on a reference cycle.
- The change applies to the JSON (`*.jmap`) parser only. The `*.mapp` parser
behaviour is unaffected.

## Specifications

- Documentation: `doc/jmapFormat.dox` (modified)
- The `jmap_interrupts` section states that `triggeredByInterrupt` may name
another interrupt register instead of giving the numeric id array.
- It states that the reference is followed transitively, so a target may
itself name a register, and that a cycle is an error.
- In the `ISR` subsection the placeholders `<handler id>` and
`<bit position>` are escaped as HTML entities (`&lt;handler id&gt;`,
`&lt;bit position&gt;`), so the doxygen build stays warning-free.
- `schemas/jmap.schema.json` (modified)
- `triggeredByInterrupt` accepts a non-empty string beside the non-empty
numeric array.
- The `interruptHandler` conditional for a chained interrupt is restricted
to `type: array`. Without this, `minItems` is ignored for a string and
every string reference would demand a handler entry; the target's own
numeric chain already triggers the requirement where one is needed.
- Every file this change touches carries the project style's full-line chrome
bars between namespace-level blocks, i.e. between class/struct declarations
and between each function implementation, adding them also where they were
missing before:
- include/JsonExtensions.h, which has none today
- include/NumericAddressedRegisterCatalogue.h
- src/JsonMapFileParser.cc
- src/NumericAddressedRegisterCatalogue.cc
- src/async/GenericMuxedInterruptDistributor.cc
- tests/executables_src/testGenericMuxedInterruptDistributor.cpp
- tests/executables_src/testJsonMapFileParser.cpp

### NumericAddressedRegisterInfo (modified) in include/NumericAddressedRegisterCatalogue.h (modified), src/NumericAddressedRegisterCatalogue.cc (modified)

- `interruptId` (modified): of type
`std::optional<std::variant<std::vector<size_t>, std::string>>`, replacing
the plain `std::vector<size_t>`.
- The variant carries the numeric id or a not yet resolved register-name
reference; `std::nullopt` is a register that is not an interrupt source.
- The JSON entry field has the same type, so the parser assigns it
unchanged, and no side table of pending references exists anywhere.
- The reference string is stored as written; the dot alt separator is
applied where the reference is resolved.
- An entry is an interrupt register when the field holds a numeric id or
a string. An empty numeric id never occurs: the parser rejects `[]` and
the constructor maps an empty vector to `std::nullopt`.
- Constructors (modified): both take the new field type,
`std::optional<std::variant<std::vector<size_t>, std::string>>`, with
`= std::nullopt` as the default of the 1D/2D-agnostic one.
- An empty numeric vector still maps to `std::nullopt`, so the `*.mapp`
parser keeps passing its possibly empty id vector.
- The 2D channel-slice construction passes the field through unchanged, so
a reference reaches the slices; it previously passed a plain vector.
- `numericInterruptId` (new):
`std::optional<std::vector<size_t>> numericInterruptId() const`.
- The numeric id, or `std::nullopt` when the field is absent or holds a
reference. Follows the value-returning style of the other optional
members and avoids handing out a pointer into the object.
- A `std::vector<size_t>` assigns into the field directly (it converts to
the numeric alternative), so no setter is needed.
- `getQualifiedAsyncId` (modified): returns
`numericInterruptId().value_or(std::vector<size_t>{})`, an empty vector when
the field is absent, as today. The empty vector is named explicitly because
`std::optional::value_or` takes a deduced forwarding reference and cannot
deduce it from a braced-empty argument.
- No throw of its own: the finalisation asserts that no reference is left,
and letting it return an empty vector keeps the accessor non-throwing.
- The header gains an `<variant>` include.
- The member is public and the header installed, so the type change breaks
source and binary compatibility for readers, in tree and downstream.
- In tree the JSON parser, the interrupt distributors and the tests are
updated; downstream code migrates to `numericInterruptId()`.

### NumericAddressedRegisterCatalogue (modified) in include/NumericAddressedRegisterCatalogue.h (modified), src/NumericAddressedRegisterCatalogue.cc (modified)

- `addRegister` and `modifyRegister` (removed): the base class already
declares both signatures, so the overrides and their bookkeeping go.
- Their doxygen blocks go with them, including the promise that
`modifyRegister` updates the interrupt bookkeeping.
- No register is ever in the interrupt indexes while its id is missing or
still a reference.
- `addInterruptBookkeeping` (removed): its body moves into
`finaliseInterruptBookkeeping`.
- `finaliseInterruptBookkeeping` (new): `void finaliseInterruptBookkeeping()`.
- Public. Called once per catalogue, after the last `addRegister` /
`modifyRegister` and after every interrupt id is resolved; the class
documentation states the obligation, as does the catalogue-taking
`NumericAddressedBackend` constructor.
- Walks the catalogue and fills `_listOfInterrupts` and
`_canonicalInterrupts` from the registers with `Access::INTERRUPT`.
- Asserts that the bookkeeping is complete, both being programming errors
rather than map errors: no register still holds a reference string, since
resolution runs before this, and no interrupt register carries an absent or
empty id.
- Rebuilds both indexes from scratch, so a repeated call is harmless and
no completion flag is needed; `clone` and `fillFromThis` copy both
indexes, so a clone is already finalised.
- `getBackendRegister` (unmodified) keeps synthesising canonical interrupt
path entries; they exist only after finalisation.

### JsonAddressSpaceEntry (modified) in src/JsonMapFileParser.cc (modified)

- `triggeredByInterrupt` (unmodified): the type already matches the new
`NumericAddressedRegisterInfo::interruptId`.
- `hasTriggeredByInterrupt` (modified): decides that an entry declares an
interrupt, from a non-empty numeric array or a string.
- Declared in the struct and defined after it in the same file.
- `interruptIdIfNumeric` and `recordInterruptReference` (removed): the field
is held in the info itself, so nothing has to be extracted or recorded.
- `fill` (modified): on a declaring entry it assigns `info.interruptId` from
`triggeredByInterrupt` as a whole, for the data and the void branch alike.
- A present empty array is a `logic_error` naming the entry: it declares
no interrupt and has no meaning, and the parser does not enforce the
schema's `minItems`.
- An empty reference string is a `logic_error` naming the entry: it names
no register, and the schema's `minLength` is likewise not enforced.
- The reference string is stored as written; the dot alt separator is
applied where the reference is resolved.
- Without a declaration the copied field is left untouched, so an entry
inheriting its parent's address and interrupt id keeps the parent's
reference without a special case. An absent declaration never
overwrites an inherited id.
- `addInfos` (modified): loses the `std::map<RegisterPath, RegisterPath>&`
parameter.
- The reference travels in the copied info, so channel and bit-range
slices, `BUF0`/`BUF1` views and entries inheriting the parent's address
carry it without per-derived-register recording.

### detail::JsonMapFileParser::Imp (modified) in src/JsonMapFileParser.cc (modified)

- `parse` (modified): after `InterruptHandlerEntry::augmentCatalogue` and
before the `selectedBy` validation loop, it resolves the references and
then finalises the bookkeeping.
- First loop collects the paths of the registers whose `interruptId` holds
a string, so the catalogue is not modified while it is iterated.
- Second loop resolves each collected register with
`resolveInterruptReference`, assigns the returned `std::vector<size_t>`
to `interruptId` and writes the register back with `modifyRegister`.
- `finaliseInterruptBookkeeping()` then builds the interrupt indexes.
- Runs after the augmentation, so the implicit `ISR` ids exist and a
derived register can be a target.
- Runs before the `selectedBy` validation and the metadata handling, so
every interrupt id and canonical path is final before either walks the
catalogue.
- `resolveInterruptReference` (modified, file-local):
`std::vector<size_t> resolveInterruptReference(const RegisterPath& targetPath,`
`const NumericAddressedRegisterCatalogue& catalogue)`.
- Returns the numeric id of the register the given path names.
- The reference-map parameter goes, since the field carries the target name;
the target is read through the catalogue instead.
- Follows the chain of named targets while keeping the already visited
target paths, so it continues into a target that itself holds a reference.
- A path that repeats throws a `logic_error`, so a self-reference or a cycle
is an error instead of a recursion without end.
- Converts the stored name to a `RegisterPath` and sets the dot alt
separator, so a dot-separated target resolves, as for `selectedBy`.
- A missing target, a non-interrupt target and a repeated path throw a
`logic_error` naming the registers involved.

### TraditionalMapFileParser (modified) in src/TraditionalMapFileParser.cc (modified)

- `parse` (modified): calls `finaliseInterruptBookkeeping()` on the parsed
catalogue before returning it.
- The `*.mapp` parser writes numeric ids only, so nothing needs resolving,
but the catalogue now requires the explicit finalisation step.

### adl_serializer (modified) in include/JsonExtensions.h (modified)

- `adl_serializer<std::optional<T>>` (unmodified in behaviour): a null value
and a missing field give `std::nullopt`, and `std::nullopt` is written as
null.
- `WITH_DEFAULT` assigns the default-constructed member for a missing key,
so the absent case rests on that default, not on the member being left
untouched. This is why `doubleBuffering` works today.
- `adl_serializer<std::variant<Ts...>>` (new): mirrors that support for the
new field.
- `to_json` writes the active alternative; a variant always holds a value,
so nothing is written as null.
- `from_json` takes the first alternative that parses, in declaration
order, skips a null explicitly and reports an error when no alternative
matches.
- Both serialisers carry documentation stating that nlohmann's
template-specialisation lookup selects them, plus one sentence per member
function on when it is called and what it does.
- The variant specialisation is generic, so it applies to every
`std::variant` serialised in a translation unit including this header;
the `triggeredByInterrupt` field is currently the only such use.
- The member functions are declared in the specialisation and defined after
it; as templates the definitions stay in the header.
- Composed on the field: absent or null gives `std::nullopt`, an array the id
vector, a string the register name. With both mappings the
`NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT` list stays unchanged (checked
by compilation).

### async::GenericMuxedInterruptDistributor (modified) in include/async/GenericMuxedInterruptDistributor.h (modified), src/async/GenericMuxedInterruptDistributor.cc (modified)

- `augmentCatalogue` (modified): builds the child id as a local
`std::vector<size_t>` from the controller id plus the bit position and
assigns it to the field, which converts to the numeric alternative.
Replaces the append onto the member, which a variant does not offer.
- It runs before the finalisation, so the registers it turns into
interrupt registers are indexed there rather than at the call.
- The `<path>/ISR` placeholder in the class documentation is escaped as an
HTML entity, so doxygen does not report it as an unsupported tag.

### Alternatives considered

- A side map of pending references filled while `addInfos` runs: rejected.
It duplicates the field, needs one recording point per derived register
and that recording is where the earlier defects were.
- Keeping the empty-id guard of the removed per-register bookkeeping helper:
rejected. The finalisation asserts the empty-id case once for the whole
catalogue, so a guard per mutation would be dead code.
- A specialisation of `adl_serializer` for the concrete variant type:
rejected, would tie the mapping to this one field, whereas the generic
`adl_serializer<std::variant<Ts...>>` mirrors the existing `std::optional`
support.
- A dedicated struct wrapping the two forms (the `SelectedBy` idiom):
rejected. `SelectedBy` pairs a path with a value and is all-or-nothing,
whereas `triggeredByInterrupt` has three states (absent, id array, name);
`std::optional<std::variant<...>>` expresses that directly.
- A string element inside the id array (e.g. `[3, 0, "REG"]`): rejected. A
name yields a whole id, so mixing it with numeric components has no
meaning.
- Resolving when the interrupt distributor is created (runtime): rejected.
The id must exist at parse time, as in CR-019.
- Requiring the target to be void: rejected. The trigger does not depend on
the target's data.
- Resolving before the augmentation: rejected. Implicit `ISR` ids are known
only after it.
- A `finalised` flag guarding the indexes: rejected. It only turns a missed
call into a different error message; the obligation is documented on the
method and on the catalogue-taking backend constructor, and the parsers
that produce complete catalogues are tested.

## Test plan

- `tests/executables_src/testJsonMapFileParser.cpp` (modified)
- Existing assertions that read `interruptId` move to
`numericInterruptId()`, so no test compares the field with a bare id
vector. Mechanical, but it touches most interrupt assertions in the
file.
- Void referring register: a `type: void` register declaring its
`triggeredByInterrupt` as a string carries the target's resolved id,
appears in `getListOfInterrupts()` and resolves through the canonical
interrupt path.
- Covers the void referrer requirement and the assignment in `fill`.
- Reference to a void interrupt register: the referring data register
carries the resolved id, appears in `getListOfInterrupts()` and resolves
through the canonical interrupt path.
- Covers the string form and the resulting bookkeeping (Requirements;
the `NumericAddressedRegisterInfo` and catalogue specifications).
- Reference to a data-carrying interrupt register: same assertions, with a
non-void target.
- Covers the data-target requirement (Requirements).
- Reference to an implicit `ISR`-derived child: the reference resolves
after the augmentation.
- Covers the resolution ordering (Requirements; `Imp::parse`).
- Forward reference: a target declared later in the file resolves.
- Covers the post-pass resolution (Requirements).
- Reference chain: two references resolve to the final id.
- Covers transitive resolution (Requirements; `resolveInterruptReference`).
- Cycle and self-reference: parse-time `logic_error`.
- Covers the cycle rule (Requirements).
- Missing target and non-interrupt target: parse-time `logic_error`.
- Covers the error cases (Requirements).
- Channel slice of a 2D referring register: the slice carries the resolved
id.
- Covers the reference travelling in the copied info (`addInfos`).
- Entry inheriting its parent's address from a referring parent: it
resolves to the parent's id.
- Covers the inherited field in `fill` (`JsonAddressSpaceEntry`).
- Empty `triggeredByInterrupt` array: parse-time `logic_error`, for a
data register and a void register alike.
- Covers the empty-array requirement; the schema already forbids it, but
the parser does not validate against the schema.
- Empty reference string: parse-time `logic_error` naming the entry.
- Covers the empty-name requirement, likewise outside the schema check.
- `tests/executables_src/testGenericMuxedInterruptDistributor.cpp` (modified)
- End to end: a data register referring by name is notified when the
target's `ISR` bit is set (dummy backend).
- Covers the runtime behaviour (Requirements); the parser test has no
backend, and the augmentation plus finalisation path is only exercised
with a real distributor.
- `tests/executables_src/testMapFileParser.cpp` (modified)
- The `*.mapp` parser still yields every interrupt register in
`getListOfInterrupts()` and through its canonical interrupt path.
- Its `interruptId` assertion (line 26) likewise reads
`numericInterruptId()`.
- Covers the finalisation call in `TraditionalMapFileParser::parse`,
which no other test reaches for this parser.
- `tests/executables_src/testNumericAddressedRegisterCatalogue.cpp` (new)
- Finalisation builds the interrupt list and the canonical interrupt
paths.
- Covers `finaliseInterruptBookkeeping` directly; the parsers cannot show
the index contents in isolation.
- A register with a registered and a derived interrupt is indexed once,
and the indexes survive `clone()`.
- Covers the rebuild and the copied indexes.

## Deferred issue

- DI-1 [DONE] A referring register without an address is never recorded in the
  interrupt-reference map, so its string 'triggeredByInterrupt' is silently
  ignored. 'recordInterruptReference' is called only in the address-bearing
  branch of addInfos (src/JsonMapFileParser.cc:510, and for slices 556/583).
  Every address-less entry goes through the 'else if(representation.type !=
  representationNotSet)' branch (src/JsonMapFileParser.cc:604-621), which never
  records the entry's own 'triggeredByInterrupt'. Because the schema and 'fill'
  force a void register to have no address and a 'type: void' representation,
  every void referring register takes that branch: it keeps an empty interrupt
  id, is absent from 'getListOfInterrupts()', has no canonical interrupt path,
  and no parse error is raised. The same entry with a numeric id works, so a
  string reference and the equivalent array are not interchangeable as required,
  and the explicit requirement 'The referring register may be void or
  data-carrying' is violated.
- DI-2 [DONE] The fallback added in the address-less branch
  (src/JsonMapFileParser.cc:612-620) assumes an empty interrupt id always means
  'inherits from a string-form parent'. An address-less entry that declares its
  own string reference is therefore not recorded for itself: it is bound to the
  parent's target (wrong id) or, when the parent carries no reference, silently
  dropped. The branch must distinguish 'no own reference, inherit from parent'
  from 'own reference' (and record the entry's own reference).
- DI-3 [DONE] The test plan (tests/executables_src/testJsonMapFileParser.cpp)
  has no case for a void referring register - its first case uses a
  data-carrying referrer - so the defect above is not detected. A test for a
  void register declared with a string 'triggeredByInterrupt' is needed once the
  implementation is fixed.
- DI-4 [DONE] tests/executables_src/testGenericMuxedInterruptDistributor.cpp:804
  uses getpid() but the file does not include <unistd.h> directly; it relies on
  a transitive include. The sibling
  tests/executables_src/testJsonMapFileParser.cpp includes <unistd.h> directly
  for the same call. Add the include (in the C system-header group).
- DI-5 [DONE] Code and test comments carry the earlier change-request number as
  historical information, which the project rule 'No historical information'
  forbids, and no other file in the project references a CR number in code:
  src/NumericAddressedRegisterCatalogue.cc:307 ('CR-019 always knew the id') and
  tests/executables_src/testJsonMapFileParser.cpp:1855 ('... after CR-019 has
  augmented the implicit interrupt ids ...'). Describe the behaviour/ordering
  instead of naming the earlier change request.
- DI-6 [DONE] An empty 'triggeredByInterrupt' array is now treated as a declared
  interrupt instead of as the previous empty state. The old test was
  'triggeredByInterrupt.empty()', which was true both for an absent field and
  for a present '[]'; it was replaced by '!triggeredByInterrupt.has_value()'
  (src/JsonMapFileParser.cc:319-327, and again for the void case at :410), and
  the nlohmann WITH_DEFAULT mapping gives '[]' an optional that holds an empty
  id vector. Consequences of a present '[]': a data register silently becomes
  INTERRUPT (read-only with wait_for_new_data) instead of the registered access
  or the read-write default, and a void register silently parses instead of the
  previous clear error 'Void-typed register ... needs triggeredByInterrupt
  entry'. Such a register keeps an empty interrupt id, so it is absent from
  getListOfInterrupts() and has no canonical interrupt path. Requesting it with
  wait_for_new_data reaches NumericAddressedBackend.cc:147,
  registerInfo.getQualifiedAsyncId().front(), i.e. front() on an empty vector
  (undefined behaviour) instead of a clean error. The schema forbids an empty
  array (minItems 1), but the parser does not validate against the schema.
- DI-7 [FOLDED-IN] Doxygen build warnings for angle-bracketed placeholders:
  doc/jmapFormat.dox:332 writes `&lt;handler id&gt;` and `&lt;bit position&gt;`
  as plain text and include/async/GenericMuxedInterruptDistributor.h:63 writes
  `&lt;path&gt;/ISR`, which doxygen reports as 'Unsupported xml/html tag'.
  Escape them as HTML entities (&lt;handler id&gt;, &lt;bit position&gt;,
  &lt;path&gt;/ISR) so the documentation build is warning-free.
- DI-8 [FOLDED-IN] include/JsonExtensions.h carries no doxygen documentation at
  all: both the pre-existing adl_serializer<std::optional<T>> and the new
  adl_serializer<std::variant<Ts...>> need a concise comment stating that
  nlohmann's template-specialisation lookup selects them, and one short sentence
  per member function saying when it is called and what it does.
- DI-9 [FOLDED-IN] In include/JsonExtensions.h the new
  adl_serializer<std::variant<Ts...>> members to_json/from_json are defined
  inside the struct body; they must be declared in the struct and defined out of
  it (as templates the definitions stay in the header). The pre-existing
  std::optional specialisation stays unchanged.
- DI-10 [FOLDED-IN] In src/JsonMapFileParser.cc the member function added by
  CR-020 is defined inside the JsonAddressSpaceEntry struct body:
  hasTriggeredByInterrupt (:319). Declare it in the struct and define it after
  it in the same file; pre-existing inline definitions such as fill and addInfos
  stay as they are.
- DI-11 [FOLDED-IN] The project coding style requires full-line '/*****/' chrome
  bars between namespace-level blocks, i.e. between class/struct declarations
  and between each function implementation, never inside a declaration or
  implementation body. Several of the files CR-020 touches lack them, so add
  them also where they were missing before. Scope: include/JsonExtensions.h,
  include/NumericAddressedRegisterCatalogue.h, src/JsonMapFileParser.cc,
  src/NumericAddressedRegisterCatalogue.cc,
  src/async/GenericMuxedInterruptDistributor.cc,
  tests/executables_src/testGenericMuxedInterruptDistributor.cpp and
  tests/executables_src/testJsonMapFileParser.cpp. Not in doc/jmapFormat.dox or
  schemas/jmap.schema.json.
- DI-12 [FOLDED-IN] Revision request: hold the not yet resolved reference in
  NumericAddressedRegisterInfo::interruptId itself (std::optional of
  std::variant<std::vector<size_t>, std::string>) instead of a side map
  collected while parsing. Removes recordInterruptReference, the map parameter
  of addInfos and the per-derived-register recording.
- DI-13 [FOLDED-IN] Revision request: replace the per-mutation interrupt
  bookkeeping with one public finalisation step called after the catalogue is
  filled and all ids are resolved; addRegister and modifyRegister no longer
  bookkeep. Both map file parsers call it once.
- DI-14 [FOLDED-IN] Revision request: an empty interrupt id is a map error
  instead of a register silently left out of the interrupt indexes. A present
  empty 'triggeredByInterrupt' array is rejected in fill; the constructor maps
  an empty vector to std::nullopt, so no empty id reaches the field, and the
  finalisation asserts that none is present.
- DI-15 [FOLDED-IN] Specifications lost the two non-class change sections during
  the design revision: `doc/jmapFormat.dox` (already changed: the
  `jmap_interrupts` text at doc/jmapFormat.dox:301-313) and
  `schemas/jmap.schema.json` (already changed: the string alternative and
  `minLength`, schemas/jmap.schema.json:111-123, plus the `interruptHandler`
  conditional restricted to `type: array` at :71-92). Both were present in the
  first commit's Specifications and the CR still changes them, so the plan no
  longer states them and later phases cannot check them.
- DI-16 [FOLDED-IN] DI-7 is only partly folded: the `<path>/ISR` escaping is
  described in the `async::GenericMuxedInterruptDistributor` section, but the
  required escaping of `<handler id>` and `<bit position>` in
  doc/jmapFormat.dox:332 has no place in the Specifications at all (no section
  for that file).
- DI-17 [FOLDED-IN] DI-11 is not folded: nothing in Requirements, Specifications
  or Test plan states the full-line chrome-bar rule for the touched files
  (include/JsonExtensions.h currently has none), although the item is marked
  FOLDED-IN.
- DI-18 [FOLDED-IN] DI-14 is only partly folded and the document contradicts
  itself: the `finaliseInterruptBookkeeping` bullet asserts only that 'no
  register still holds a reference', while DI-14 asks the finalisation to assert
  that no empty interrupt id is present; the 'Alternatives considered' bullet
  then calls an empty-id guard 'dead code'. The finalisation assertion must
  cover both a leftover reference and an absent/empty id.
- DI-19 [FOLDED-IN] The modified `resolveInterruptReference` is under-specified:
  the bullet says it returns the id 'recursively' and loses the reference-map
  parameter, but neither the new signature nor the cycle-detection mechanism is
  stated. With the reference now stored in the field, a self-reference or a
  two-register cycle recurses forever unless the already visited target paths
  are tracked (the current code uses a chain vector,
  src/JsonMapFileParser.cc:774-800). The signature and the visited-chain rule
  must be specified.
- DI-20 [FOLDED-IN] The `getQualifiedAsyncId` bullet prescribes
  `numericInterruptId().value_or({})`, which does not compile:
  `std::optional::value_or` takes a deduced forwarding reference
  (`template<class U> T value_or(U&&)`), so `{}` cannot be deduced. It must name
  the empty vector explicitly.
