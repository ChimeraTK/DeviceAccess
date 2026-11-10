# CR-020: Register-name references in triggeredByInterrupt

Synopsis: A register may state its interrupt id as the name of another
interrupt register instead of a numeric id array. The referring register
takes the resolved id of that register, declared explicitly or derived
implicitly.

Depends on: CR-019

Status: STYLE REVIEWED

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
- The change applies to the JSON (`*.jmap`) parser only. The `*.mapp` parser
is unaffected.
- A string reference and the equivalent numeric array are interchangeable,
and the referring register behaves exactly like one declared with the
resolved id.
- Registers resolving to the same id are notified together and share the
canonical interrupt path, as CR-019 already allows for a written id.
- The field stays mutually exclusive with `access` and remains required for a
void register.
- An error while parsing the map file when the target does not exist, when
the target is not an interrupt register, or on a reference cycle.

## Specifications

- Documentation: `doc/jmapFormat.dox` (modified)
- The `jmap_interrupts` section states that `triggeredByInterrupt` may name
another interrupt register instead of giving the numeric id.
- `schemas/jmap.schema.json` (modified)
- The schema accepts a non-empty string for `triggeredByInterrupt` beside
the numeric array.
- The `interruptHandler` conditional for a chained interrupt is restricted
to arrays (`type: array`).
- Without this, `minItems` is ignored for a string and every string
reference would demand a handler; the target's own numeric chain
already triggers the requirement where one is needed.

### JsonAddressSpaceEntry (modified) in src/JsonMapFileParser.cc (modified)

- `triggeredByInterrupt` (modified): of type
`std::optional<std::variant<std::vector<size_t>, std::string>>`, replacing
the plain `std::vector<size_t>`.
- `optional` carries absence, `variant` the present value; no `nullptr`
alternative is involved.
- Accepted forms: a non-empty array of non-negative integers as today, or
a non-empty string parsed as a `RegisterPath`.
- An empty array counts as absent: the interrupt decision rests on a
present array with at least one element, so the previous empty state is
preserved and no id-less interrupt register is created.
- The three existing uses map as: absent or an empty array is empty, and
the non-void and void assignments to `info.interruptId` read the id vector
of a present, non-empty array.
- Serialisation: `include/JsonExtensions.h` (modified) gains an
`adl_serializer<std::variant<Ts...>>`; the existing
`adl_serializer<std::optional<T>>` covers absence.
- `to_json` writes the active alternative of the `variant`; `from_json`
takes the first alternative that parses, in declaration order, and
reports an error when none does. A variant always holds a value, so
this mapping neither writes nor accepts null.
- The `optional` mapping is the existing one: a missing field (the
`WITH_DEFAULT` macro leaves the member untouched) and a null value
give `std::nullopt`, and `std::nullopt` is written as null. This is
why `doubleBuffering` works today.
- Composed: absent or null gives `std::nullopt`, an array the id
vector, a string the register name. With both mappings the
`NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT` list stays unchanged
(checked by compilation).
- `fill` (modified): on the string form it sets interrupt access and leaves
`interruptId` empty, so the register is added before its id is resolved.
- The reference string and the reference map keys are normalised with
`setAltSeparator(".")`, as the `selectedBy` handling does, so a
dot-separated target resolves.
- `addInfos` (modified): takes a `std::map<RegisterPath, RegisterPath>&`
(referring register path to target name).
- In the address-bearing branch it records an entry for a string-form
register and for the channel and bit-range slices derived from it, so
every referring register there is covered.
- In the address-less branch, which carries every void register and every
entry inheriting its parent's address, it records the entry's own
reference under the entry's path, so a void referring register is
covered as well.
- Only an address-less entry declaring no reference of its own but
inheriting an interrupt id takes over its string-form parent's
unresolved reference, so the child's id resolves with its parent's.
- The branch decides this from `triggeredByInterrupt`, not from an
empty `interruptId`: `fill` leaves the id empty both for an own
string reference and for inheritance from a string-form parent.
- The map is passed down from `parse`.

### NumericAddressedRegisterCatalogue (modified) in src/NumericAddressedRegisterCatalogue.cc (modified)

- `addInterruptBookkeeping` (modified): returns without bookkeeping when
`interruptId` is empty.
- CR-019 guards only on the access, but a string-form register is added
with interrupt access before its id is known. The id is written later
through `modifyRegister`, which then runs the bookkeeping.
- This relaxes a helper introduced by CR-019, whose own callers always pass
a non-empty id; the empty case is created by CR-020 alone.

### detail::JsonMapFileParser::Imp (modified) in src/JsonMapFileParser.cc (modified)

- `parse` (modified): immediately after the CR-019 augmentation, and before
the `selectedBy` validation loop, it resolves every collected reference and
writes the id back with `NumericAddressedRegisterCatalogue::modifyRegister`.
- Runs after `InterruptHandlerEntry::augmentCatalogue`, so the implicit
`ISR` ids exist and a derived register can be a target.
- Runs before the `selectedBy` validation and the metadata handling, so
every interrupt id is final before either walks the catalogue.
- `resolveInterruptReference` (new, file-local): follows a reference chain to
the final register and returns its id.
- A missing target, a non-interrupt target and a cycle throw a
`logic_error` naming the registers involved.

### Alternatives considered

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
- Resolving before the CR-019 augmentation: rejected. Implicit `ISR` ids are
known only after augmentation.

## Test plan

- `tests/executables_src/testJsonMapFileParser.cpp` (modified)
- Void referring register: a `type: void` register declaring its
`triggeredByInterrupt` as a string carries the target's resolved id,
appears in `getListOfInterrupts()` and resolves through the canonical
interrupt path.
- Covers the void referrer requirement and the address-less recording
in `addInfos` (Requirements; Specifications).
- Reference to a void interrupt register: the referring data register
carries the resolved id, appears in `getListOfInterrupts()` and resolves
through the canonical interrupt path.
- Covers the string form and the resulting bookkeeping (Requirements;
the `JsonAddressSpaceEntry` and catalogue specifications).
- Reference to a data-carrying interrupt register: same assertions, with a
non-void target.
- Covers the data-target requirement (Requirements).
- Reference to an implicit `ISR`-derived child (CR-019): the reference
resolves after the augmentation.
- Covers the resolution ordering (Requirements; `Imp::parse`).
- Forward reference: a target declared later in the file resolves.
- Covers the post-pass resolution (Requirements).
- Reference chain: two references resolve to the final id.
- Covers transitive resolution (Requirements).
- Cycle and self-reference: parse-time `logic_error`.
- Covers the cycle rule (Requirements).
- Missing target and non-interrupt target: parse-time `logic_error`.
- Covers the error cases (Requirements).
- Channel slice of a 2D referring register: the slice carries the resolved
id.
- Covers the derived-register coverage in `addInfos` (Specifications).
- `tests/executables_src/testGenericMuxedInterruptDistributor.cpp` (modified)
- End to end: a data register referring by name is notified when the
target's `ISR` bit is set (dummy backend), confirming the resolved id
reaches the distributor.
- Covers the runtime behaviour (Requirements); the parser test has no
backend.

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
- DI-6 [NEW] An empty 'triggeredByInterrupt' array is now treated as a declared
  interrupt instead of as the previous empty state. The old test was
  'triggeredByInterrupt.empty()', which was true both for an absent field and
  for a present '[]'; it was replaced by '!triggeredByInterrupt.has_value()'
  (src/JsonMapFileParser.cc:334, and again for the void case at :397), and the
  nlohmann WITH_DEFAULT mapping gives '[]' an optional that holds an empty id
  vector. Consequences of a present '[]': a data register silently becomes
  INTERRUPT (read-only with wait_for_new_data) instead of the registered access
  or the read-write default, and a void register silently parses instead of the
  previous clear error 'Void-typed register ... needs triggeredByInterrupt
  entry'. Such a register keeps an empty interrupt id, so it is absent from
  getListOfInterrupts() and has no canonical interrupt path, i.e. it violates
  the invariant stated in the NumericAddressedRegisterCatalogue specification
  that an empty id exists only transiently for a string-form register (the id is
  written later through modifyRegister). Requesting it with wait_for_new_data
  reaches NumericAddressedBackend.cc:147,
  registerInfo.getQualifiedAsyncId().front(), i.e. front() on an empty vector
  (undefined behaviour) instead of a clean error. The schema forbids an empty
  array (minItems 1), but the parser does not validate against the schema, and
  the specification bullet 'The three existing uses map as: absent is empty'
  intends the old empty state (absent or empty array) to map to absence.
  Suggested fix (no specification change needed): treat a present-but-empty id
  array as absent in fill/interruptIdIfNumeric, i.e. base the interrupt decision
  on a present array with at least one element; alternatively reject it with a
  clear logic_error.
