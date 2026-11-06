# CR-019: Implicit interrupt registers from INTC ISR bit fields

Synopsis: The interrupts of an `INTC` interrupt controller can only be
referred to by their numeric id. The 1-bit children of the controller's
`ISR` register shall implicitly become interrupt registers carrying the id
`[<handler id>, <bit position>]`, so that interrupts can be referred to by
name.

Status: READY TO TEST (from IMPLEMENTATION REVIEWED)

## Requirements

Aspect: interrupts derived from the `ISR` of an `INTC` controller.

- The derivation and the parse-time validation below apply to the JSON
(`*.jmap`) map files only. The traditional (`*.mapp`) parser is unaffected
and keeps its current behaviour, so `*.mapp` fixtures with `INTC` entries
without an `ISR` (e.g. `tests/irq_test.mapp`) continue to parse.
- An `INTC` handler entry with qualified interrupt id `I` and controller
path `P` makes each child register of `P/ISR` an interrupt register. A
child qualifies when it is one 1-bit bit field of the ISR word: one channel
of width 1 bit, one element, and the same bar and address as `ISR`.
- The child's interrupt id is `I` followed by the child's bit position in
the ISR word.
- The child keeps its path, name, address, size and bit position; only its
access changes to interrupt.
- The derived registers behave like declared interrupt registers: readable
with `wait_for_new_data`, notifying for the ISR bit the child names, part
of the backend's list of interrupts, and reachable through their canonical
interrupt path.
- `ISR` must be a scalar register: one element, one channel, no `channels`
dictionary. A multi-element or multi-channel `ISR` is an error while parsing
the map file: a channel slice below `ISR` would be indistinguishable from a
bit field, so no channels appear below `ISR`.
- `ISR` must not be double-buffered: its `BUF0`/`BUF1` views are children that
are not bit fields, so a double-buffered `ISR` is an error while parsing the
map file.
- `ISR` either has no children at all, or only qualifying children.
- A child that is not a qualifying bit field, i.e. a wider, multi-channel or
multi-element child, or one at its own address, is an error while parsing
the map file. The handler interprets `ISR` as one status bit per interrupt,
so any other child has no defined meaning.
- Deeper levels below `ISR` are not allowed: a child that has children of its
own is an error while parsing the map file.
- Every handler entry is processed, primary and nested. A nested entry uses
its own `path` and its full id.
- Descriptor parsing and validation move to parse time for the parts the
augmenter needs, which is stricter than today: a handler entry was previously
interpreted only when a handler used it, so an unused entry was never
validated.
- An `INTC` handler entry whose `P/ISR` does not exist is an error while
parsing the map file, for used and unused entries alike. An unused but valid
entry is now merely validated rather than ignored, so it still parses; only a
genuinely invalid entry, such as a missing `P/ISR`, makes a map that parsed
before fail to parse.
- The controller `path` of every `INTC` handler entry is read and validated at
parse time, no longer only when a handler is used.
- The option-register validation of an entry is not part of this and still runs
only when a handler uses it, so an unused entry with an invalid option
combination fails only then.
- An `ISR` without any child is not an error and changes nothing.
- Two children covering the same bit position are an error while parsing the
map file.
- A derived id that is already present is not an error: two registers may
share one interrupt id and are then notified together by the same `ISR` bit.
- Only the built-in `INTC` controller type augments the catalogue. The other
built-in type, `dummy`, is left untouched and only reaches the metadata.
- Augmentation is a capability of a controller type, not of `INTC` alone: the
mechanism must let a controller type supply its own augmentation.
- The factory offers no public way to register a further controller type yet,
so only the built-in `INTC` augments for now. Adding that registration is
deferred to a separate change request; the design assumes it will exist.

## Specifications

- Documentation: `doc/jmapFormat.dox` (modified).
- The `jmap_interrupts` section gains a note that the 1-bit child bit fields
of `<path>/ISR` become interrupt registers with the id
`[<handler id>, <bit position>]`, and that an `INTC` `ISR` must be a scalar
register without channels or deeper levels.
- The sentence that the parser passes an interrupt controller entry on to the
metadata verbatim without interpreting it (around line 267) is corrected: an
`INTC` entry is now interpreted at parse time to augment the catalogue,
while other handler types keep the verbatim behaviour.

### NumericAddressedRegisterCatalogue (modified) in include/NumericAddressedRegisterCatalogue.h (modified), src/NumericAddressedRegisterCatalogue.cc (modified)

- `void modifyRegister(NumericAddressedRegisterInfo const& info)` (new)
- Replaces a register in place and keeps the insertion order.
- It updates the derived interrupt bookkeeping first and then calls the
base `modifyRegister`.
- Reason: the base primitive touches neither set, so the derived registers
would otherwise be missing from `getListOfInterrupts()` and their canonical
interrupt paths would not resolve.
- `void addInterruptBookkeeping(NumericAddressedRegisterInfo const& info)`
(new)
- Extracts the interrupt bookkeeping of `addRegister` into a private
helper.
- Both `addRegister` and `modifyRegister` call the helper, so the logic
stays in one place.
- The helper inserts the interrupt id into `_listOfInterrupts` and the
canonical interrupt paths `!I`, `!I:M`, ... into `_canonicalInterrupts`.
- The helper only inserts, so it does not support retyping a register away
from interrupt access.
- A pre-existing interrupt id is not an error: the insertion is idempotent,
so two registers can share one id.
- The derived `modifyRegister` hides the non-virtual base method, so callers
on the concrete type also run the bookkeeping; for a non-interrupt register
the helper does nothing. This mirrors the existing non-virtual `addRegister`
override.
- Dependencies: none new.

### async::MuxedInterruptDistributorFactory (modified) in include/async/MuxedInterruptDistributor.h (modified), src/async/MuxedInterruptDistributor.cc (modified)

- Registry record (new)
- One entry per controller type, so the existing creator and the optional
catalogue augmenter share one lookup key and one registration point.
- Replaces the `_creatorFunctions` map.
- The optional augmenter is the extension point for further controller
types: a type that augments supplies its own. Only the built-in `INTC`
registers one today; a public registration API is a separate change
request (see Requirements).
- The augmenter member has this type:
`std::function<void(std::vector<size_t> const&, std::string const&,
NumericAddressedRegisterCatalogue&)>`; it is empty for a type that does
not augment the catalogue.
- Private constructor (modified)
- Registers the built-in types: `INTC` with its creator and augmenter,
`dummy` with its creator only.
- `void augmentCatalogue(std::vector<size_t> const& interruptId,
std::string const& controllerType, std::string const& controllerDescription,
NumericAddressedRegisterCatalogue& catalogue)` (new)
- Dispatches one handler entry to the augmenter of its controller type.
- Looks the record up by `controllerType` and calls its augmenter with the
id and the description.
- An unknown type and a type with an empty augmenter are skipped, so the
dispatcher is a no-op for `dummy` and for any type that is not
registered. An unknown type stays an error at distributor creation.
- Takes the controller type and the description as strings, so the header
needs neither `nlohmann::json` nor the catalogue's definition; the
catalogue is only forward-declared.
- `getInterruptControllerNameAndDescriptionFromCatalogue` (modified)
- Extracts the controller type and the description through the shared free
function `interruptControllerNameAndDescription`, so the convention lives
in one place.
- Dependencies: the augmenter of `INTC`,
`async::GenericMuxedInterruptDistributor::augmentCatalogue`, and the shared
`interruptControllerNameAndDescription`.

### async::GenericMuxedInterruptDistributor (modified) in include/async/GenericMuxedInterruptDistributor.h (modified), src/async/GenericMuxedInterruptDistributor.cc (modified)

- `static void augmentCatalogue(std::vector<size_t> const& interruptId,
std::string const& controllerDescription,
NumericAddressedRegisterCatalogue& catalogue)` (new)
- The augmenter `MuxedInterruptDistributorFactory` stores for `INTC`.
- Reads `path` from the controller description by reusing the parsing and
validation of `create`, so descriptor errors keep their messages and gain
the handler id.
- Descriptors of all entries are validated at parse time, no longer only
when a handler is used.
- Finds `path / "ISR"` in the catalogue; the separators are synonymous. A
missing register throws a `logic_error` naming the handler id and the
register.
- A non-scalar `ISR`, i.e. one with several elements or with a channel
layout, throws a `logic_error`: a channel slice below `ISR` would be
indistinguishable from a bit field, so no channels may appear below
`ISR`. A scalar register carries exactly one channel that spans the whole
element word (`bitOffset` 0 and `width` equal to the element pitch),
whereas a register declared with a `channels` dictionary carries one
channel per named channel, whose `width` is the channel's own width. The
check requires the single channel to span the element word, so it also
rejects a `channels` dictionary that happens to declare exactly one
channel.
- Children are the catalogue registers with exactly one path component
more than `ISR`.
- A register more than one path component below `ISR` throws a
`logic_error`, so no deeper levels are allowed. A first pass over the
registers below `ISR` rejects any deeper register, so the rule holds
independently of the order in which the catalogue is enumerated; the
second pass then validates and retypes the direct children.
- A child qualifies as one channel of width 1, one element, at the same bar
and address as `ISR`. The bit position is the channel's bit offset, which
for a bit field is its `representation.bitShift`.
- A child that is not such a bit field throws a `logic_error`.
- Retyping is in place: a copy of the register info with interrupt access
and the interrupt id, written back through the catalogue's
`modifyRegister`.
- Channels, address, size, bit-range flag, path and the `BUF0`/`BUF1`
views stay as they are.
- Two children with the same bit position throw a `logic_error`.
- An `ISR` without children returns without a change.
- The catalogue parameter is by reference, so the header needs only a forward
declaration of `NumericAddressedRegisterCatalogue`, not its definition.
- Dependencies: `NumericAddressedRegisterCatalogue::modifyRegister` (new) and
`GenericMuxedInterruptDistributor::create`.

### free function `interruptControllerNameAndDescription` (new) in include/JsonExtensions.h (modified)

- `std::pair<std::string, std::string> interruptControllerNameAndDescription(
nlohmann::json const& descriptor)` (new)
- Returns the controller type (the first key of the descriptor) and its
dumped value (the description).
- Used by both
`MuxedInterruptDistributorFactory::getInterruptControllerNameAndDescriptionFromCatalogue`
and `detail::InterruptHandlerEntry::augmentCatalogue`, so the convention
lives in one place.
- Dependencies: none new.

### detail::InterruptHandlerEntry (modified) in src/JsonMapFileParser.cc (modified)

- `void forEachEntry(std::vector<size_t> const& intId,
std::function<void(std::vector<size_t> const&, nlohmann::json const&)> const&
visitor) const` (new)
- The single traversal of the entry tree: when `intId` is non-empty it visits
the entry with its qualified id and its `raw` object, then recurses into
`subhandler`, appending `std::stoll(subIntId)` to the id.
- Holds the `subhandler` layout and the qualified-id construction in one
place, shared by `fill` and `augmentCatalogue`.
- `void fill(const std::vector<size_t>& intId, MetadataCatalogue& metadata)
const` (modified)
- Becomes a `forEachEntry` call whose visitor adds `"!" + id.dump()` mapped to
`raw.dump()` to the metadata. Behaviour unchanged.
- `void augmentCatalogue(NumericAddressedRegisterCatalogue& catalogue) const`
(new)
- Calls `forEachEntry` from the root with an empty id.
- The visitor dispatches only for an entry whose controller object is
non-empty. The root entry carries an empty object as well as an empty id,
so nothing dispatches for it.
- A nesting-only entry without a controller object still reaches its
subhandlers: the traversal runs the visitor and the recursion
independently, so skipping the dispatch does not stop the walk.
- Extracts the controller type and the description through the shared free
function `interruptControllerNameAndDescription`, rather than repeating the
convention.
- A nested entry contributes its own full id, so its children carry the
subhandler's full id.
- Dependencies: the `MuxedInterruptDistributorFactory` dispatcher.

### detail::JsonMapFileParser::Imp (modified) in src/JsonMapFileParser.cc (modified)

- `parse` (modified)
- Augments once the address space is in the catalogue and the bit-range
scan has run, so `isBitRange` is final, and before catalogue and metadata
are returned.
- It calls `InterruptHandlerEntry::augmentCatalogue` for the handler entry
of the map file, so every `INTC` entry is augmented, used or not, at
parse time.
- The ids have to exist while the map file is parsed (see Requirements).
- Dependencies: `InterruptHandlerEntry::augmentCatalogue`.

### Alternatives considered

- Temporarily creating a distributor and calling a virtual augment method on
it: rejected. A distributor needs a `SubDomain` holding a backend, and its
constructor opens the ISR/IER accessors; neither exists at parse time. It
would need a parent-less construction path and the backend-dependent setup
moved out of the constructor.
- Public registration of controller types (a `registerDistributor` template
plus a base-class `augmentCatalogue` default): deferred to a separate change
request. The registry record's optional augmenter is the extension point, so
the mechanism lets a further type augment; only the public registration call
is missing.
- Requiring each controller class to carry its own name as a static member:
rejected. The name is a property of the registration, and an argument keeps
the controller classes free of it.
- A second registry in the factory beside the creator functions: rejected.
One record per type keeps one lookup key and one registration point.
- Removing and re-adding a register instead of modifying it: rejected. It
loses the insertion order of the iterated catalogue.
- Augmenting at `open()` or lazily in the backend: rejected. The interrupt
ids have to exist while the map file is parsed.
- Excluding the `BUF0`/`BUF1` buffer views of `ISR` from the child scan so a
double-buffered `ISR` still parses: rejected. The child scan enumerates the
catalogue by path component and does not special-case the buffer views, so a
double-buffered `ISR` is an error (see Requirements).
- Detecting a `channels` layout below `ISR` by `channels.size() != 1` alone:
rejected. A `channels` dictionary with a single channel leaves the register
with one channel and one element, indistinguishable from a scalar by count.
The check instead requires the single channel to span the whole element word
(`bitOffset` 0 and `width` equal to the element pitch), which a scalar
register always satisfies and a `channels` register does not, since its
channel `width` is the channel's own width. Detecting the layout by
`elementPitchBits` was rejected as well: a `channels` register may set an
explicit `pitch` that coincides with a scalar element size, while the
channel-word comparison holds in all cases.

## Test plan

- `tests/simpleJsonFile.jmap` (modified)
- Every `INTC` handler gains an `ISR` and an `IER`: `DAQ` gains both, with
named 1-bit children under its `ISR`, and subhandler `[3,1]` receives a
`MY_INTC.SUB1` block with its own `ISR` and `IER`.
- Covers the derivation for every `INTC` handler (Requirements) and the
`<path>/ISR` lookup and `IER` access in
`async::GenericMuxedInterruptDistributor` (Specifications).
- Both registers are required because the INTC distributor constructor
opens `<path>/ISR` and `<path>/IER` (default options ISR+IER) and checks
the `IER` is writeable.
- Adding the block keeps the metadata `path` of subhandler `[3,1]`
unchanged, so the existing metadata assertions stay valid. Repointing
`[3,1]` at another existing block would change the emitted `path` and
break `tests/executables_src/testJsonMapFileParser.cpp:697`.
- `tests/executables_src/testJsonMapFileParser.cpp` (modified)
- Children of an `ISR`: 1-bit children become interrupt registers with the
expected ids, unchanged path, address and bit position, present in the list
of interrupts, and resolvable through the canonical interrupt path.
- Covers the derivation and the canonical-path bookkeeping in
`NumericAddressedRegisterCatalogue` and
`async::GenericMuxedInterruptDistributor` (Specifications).
- Nested subhandler: children of a subhandler `ISR` carry the subhandler's
full id.
- Covers nested-entry handling in
`detail::InterruptHandlerEntry::augmentCatalogue` (Specifications).
- A child that is not a 1-bit value (wider, multi-channel, multi-element):
parse-time `logic_error`, with a temporary map file.
- Covers the qualifying-child requirement (Requirements).
- Deeper levels below `ISR`: a child that has a child of its own gives a
parse-time `logic_error`, with a temporary map file.
- Covers the no-deeper-levels requirement (Requirements).
- A non-scalar `ISR` (several elements, or declared with `channels`):
parse-time `logic_error`, with a temporary map file.
- Covers the scalar-`ISR` requirement (Requirements).
- Double-buffered `ISR`: a temporary map puts a `doubleBuffering` block on
`DAQ/ISR`, giving a parse-time `logic_error`.
- Covers the double-buffered-`ISR` requirement (Requirements): the `BUF0`/`BUF1`
children are not bit fields, so the child scan rejects them.
- `ISR` without any child: parses, no error, catalogue unchanged.
- Covers the empty-`ISR` requirement (Requirements).
- Missing `ISR`: parse-time `logic_error`, with a map file written to a
temporary file that lacks the `ISR`; one case removes the `ISR` of a used
handler (`DAQ`), another removes that of an unused handler entry
(`MY_INTC.SUB1`, handler `[3,1]`, referenced by no register).
- Covers the missing-`P/ISR` requirement (Requirements) for used and unused
entries alike.
- Duplicate bit position: two children with the same bit position give a
parse-time `logic_error`, with a temporary map file.
- Covers the duplicate-bit-position requirement (Requirements).
- A derived id that is already present: a declared interrupt register
carrying `triggeredByInterrupt` `[<handler id>, <bit position>]` parses
without error and both registers carry the id.
- Covers the shared-interrupt-id requirement (Requirements).
- Update the interrupt-list assertion in `TestGoodMapFileParse`:
`loi.size() == 3` with the ids at lines 701-703 to the derived ids the map
change adds, and re-check the other catalogue assertions in the same test
for the new registers.
- The map change adds derived interrupt ids to `_listOfInterrupts`, so the
existing assertion would fail.
- Consumers of `tests/simpleJsonFile.jmap` (modified as needed)
- Re-check and, if required, update the suites that also load the extended
fixture: `tests/executables_src/testDeviceToJmap.cc` (CR-005 round-trip
catalogue comparison), `tests/executables_src/testLMapBackend.cpp`
(`logicalnamemap.dmap`), `tests/executables_src/testDoubleBufferAccessor.cc`
and `tests/executables_src/testGetSetAsCoocked.cpp`.
- The fixture change adds registers and derived interrupt ids that reach
these suites; the plan previously updated only
`testJsonMapFileParser.cpp`.
- `tests/executables_src/testGenericMuxedInterruptDistributor.cpp` (modified)
- Add a jmap-based CDD for the case below: the file's current
`tests/irq_test.mapp` fixture is `*.mapp` and never derives interrupts.
Reuse `tests/simpleJsonFile.jmap` where suitable; otherwise copy the nearest
jmap carrying an `INTC` block to a temporary file and adapt it.
- Derived interrupt end to end: a `wait_for_new_data` accessor on a derived
interrupt register is notified when the corresponding ISR bit is set
(dummy backend interrupt trigger).
- Confirms the id-to-ISR-bit mapping and the runtime behaviour of the
derived registers (Requirements); the parser test does not exercise a
backend.
- `tests/executables_src/testMuxedInterruptDistributorFactory.cpp` (new)
- Dispatcher no-op: `augmentCatalogue` called with an unknown controller
type, and with the built-in `dummy` type that has no augmenter, leaves the
catalogue unchanged.
- Covers the skip of an unknown type and of a type with an empty augmenter
in `async::MuxedInterruptDistributorFactory` (Specifications).
- Creating a distributor for an unknown controller type still throws
`logic_error`.
- Guards the boundary between the dispatcher's lookup-miss (skip) and
creation's lookup-miss (throw), which the shared record lookup could
break (Specifications).
- Not covered elsewhere: the interrupt tests only declare the registered
types, `INTC` (`tests/irq_test.mapp`) and `dummy`
(`tests/testHierarchicalInterrupts.map`), while the parser test whose
map carries a backend-specific type (`MyINTC`) never opens a backend.
- The setup needs a backend whose metadata declares an unregistered type,
e.g. a dummy backend over a map with a backend-specific controller
entry. The error comes from the type lookup, before any ISR is opened.

## Deferred issue

- DI-1 [DONE] Compiler warning -Wunused-result at
  tests/executables_src/testMuxedInterruptDistributorFactory.cpp:96: the
  [[nodiscard]] return value of Device::getVoidRegisterAccessor is discarded
  inside BOOST_CHECK_THROW (clang-tidy expands the macro and points at the
  nodiscard declaration). This violates the project's no-warnings policy;
  discard the value explicitly (e.g. (void)) or restructure the check.
- DI-2 [DONE] Garbled comment sentence at src/JsonMapFileParser.cc:724-728: '...
  so the ids exist while the map file is parsed and the derived registers are
  missing from getListOfInterrupts() and their canonical interrupt paths
  otherwise would not resolve.' The clause reads as the opposite of the intent
  and the 'otherwise' is misplaced. Rewrite as two sentences, e.g. '... so the
  ids exist while the map file is parsed. Otherwise the derived registers would
  be missing from getListOfInterrupts() and their canonical interrupt paths
  would not resolve.'
- DI-3 [IMPLEMENTED] Coverage gap: the requirement 'ISR must not be
  double-buffered ... a double-buffered ISR is an error while parsing the map
  file' (Requirements; see also the matching 'Alternatives considered' entry)
  has no test. The Test plan lists every other derivation/validation case but
  omits this one, and no test exercises it (grep for doubleBuffer in
  tests/executables_src/testJsonMapFileParser.cpp only shows the pre-existing
  DAQ.DOUBLE_BUF assertions). A regression that special-cased the BUF0/BUF1
  views so a double-buffered ISR parses (the exact alternative the CR rejected)
  would pass the whole suite. Add a temporary-map case that puts a
  'doubleBuffering' block on DAQ/ISR (reusing the DAQ.DOUBLE_BUF
  enable/read-buffer registers) and expects a parse-time logic_error.
- DI-4 [IMPLEMENTED] Coverage gap: the requirement that an INTC handler entry
  whose P/ISR does not exist is an error 'for used and unused entries alike' is
  only tested for the used handler [0]. TestIsrMissing
  (testJsonMapFileParser.cpp) removes DAQ/ISR, which IS used (VOID_INTERRUPT_0
  and FD declare triggeredByInterrupt [0]), so a regression that validated only
  used entries would still pass. Add a case that removes an UNUSED handler
  entry's ISR, e.g. MY_INTC/SUB1/ISR for handler [3,1] (no register references
  [3,1]), and expects a parse-time logic_error.
