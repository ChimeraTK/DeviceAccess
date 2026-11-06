# CR-020: Register-name references in triggeredByInterrupt

Synopsis: A register may state its interrupt id as the name of another
interrupt register instead of a numeric id array. The referring register
takes the resolved id of that register, declared explicitly or derived
implicitly.

Depends on: CR-019

Status: PLANNED

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
  register. A cycle, including a register naming itself, is an error.
- References resolve after CR-019 has augmented the implicit interrupt ids,
  so a target may be a derived `ISR` child and may be declared anywhere in
  the file.
- A register below an `ISR` that declares its own `triggeredByInterrupt` is an
  error: CR-019's augmentation defines the id of an `ISR` child, so the two
  would disagree.
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

- Documentation: `doc/jmapFormat.dox` (modified),
  `schemas/jmap.schema.json` (modified).
- The `jmap_interrupts` section states that `triggeredByInterrupt` may name
  another interrupt register instead of giving the numeric id.
- The schema accepts a non-empty string for `triggeredByInterrupt` beside the
  numeric array.
- The conditional that requires an `interruptHandler` section for a chained
  interrupt is restricted to arrays (`type: array`). Without this, `minItems`
  is ignored for a string and every string reference would demand a handler;
  the target's own numeric chain already triggers the requirement where one is
  needed.

### JsonAddressSpaceEntry (modified) in src/JsonMapFileParser.cc (modified)

- `triggeredByInterrupt` (modified): a small struct carrying either the
  numeric id array or a register-name reference, replacing the plain
  `std::vector<size_t>`. This follows the `SelectedBy` idiom for a polymorphic
  JSON field, and keeps the three existing uses (`.empty()`, and the non-void
  and void assignments to `info.interruptId`) working through one member.
- Custom JSON mapping: a non-empty array of non-negative integers as today, or
  a non-empty string parsed as a `RegisterPath`.
- `fill` (modified): on the string form it sets interrupt access and leaves
  `interruptId` empty, so the register is added before its id is resolved.
- The reference string and the reference map keys are normalised with
  `setAltSeparator(".")`, as the `selectedBy` handling does, so a
  dot-separated target resolves.
- `addInfos` (modified): takes a `std::map<RegisterPath, RegisterPath>&`
  (referring register path to target name). It records an entry for a
  string-form register, for the channel and bit-range slices derived from it
  and, on the inherited-representation branch, for the child taking its
  interrupt id from its parent, so every interrupt register of a
  string-form entry is covered.
- The map is passed down from `parse`.

### NumericAddressedRegisterCatalogue (modified) in src/NumericAddressedRegisterCatalogue.cc (modified)

- `addInterruptBookkeeping` (modified): returns without bookkeeping when
  `interruptId` is empty.
- CR-019 guards only on the access, but a string-form register is added with
  interrupt access before its id is known. The id is written later through
  `modifyRegister`, which then runs the bookkeeping.

### detail::JsonMapFileParser::Imp (modified) in src/JsonMapFileParser.cc (modified)

- `parse` (modified): after the CR-019 augmentation it resolves every
  collected reference and writes the id back with
  `NumericAddressedRegisterCatalogue::modifyRegister`.
- Runs after `InterruptHandlerEntry::augmentCatalogue`, so the implicit `ISR`
  ids exist and a derived register can be a target.
- `resolveInterruptReference` (new, file-local): follows a reference chain to
  the final register and returns its id.
- A missing target, a non-interrupt target and a cycle throw a `logic_error`
  naming the registers involved.

### async::GenericMuxedInterruptDistributor (modified) in src/async/GenericMuxedInterruptDistributor.cc (modified)

- `augmentCatalogue` (modified): a child of `ISR` with interrupt access, i.e.
  one carrying its own `triggeredByInterrupt` (numeric or name), throws a
  `logic_error`.
- The derived id of an `ISR` child is defined by the augmentation, so an
  explicit one would disagree; a plain `ISR` child stays read-write and is
  unaffected.
- This is the only place the clash can be detected: the child's access is
  known before the id is resolved.

### Alternatives considered

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
   - Explicit trigger below `ISR`: a bit-field child of `ISR` carrying a
     numeric or string `triggeredByInterrupt` gives a parse-time `logic_error`.
      - Covers the `ISR`-child clash requirement (Requirements; the
        `GenericMuxedInterruptDistributor` specification).
   - Channel slice of a 2D referring register: the slice carries the resolved
     id.
      - Covers the derived-register coverage in `addInfos` (Specifications).
- `tests/executables_src/testGenericMuxedInterruptDistributor.cpp` (modified)
   - End to end: a data register referring by name is notified when the
     target's `ISR` bit is set (dummy backend), confirming the resolved id
     reaches the distributor.
      - Covers the runtime behaviour (Requirements); the parser test has no
        backend.
