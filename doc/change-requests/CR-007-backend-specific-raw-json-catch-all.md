# CR-007: Raw-json DMA channel configuration in the JMAP file

Synopsis: Add an optional, opaque `dmaChannels` top-level section to the JMAP
file format, preserved as raw `nlohmann::json` and exposed to backend-specific
subclasses of `NumericAddressedBackend`. The generic parser validates only the
shallow envelope; the content is interpreted by the specific backend (first
consumer: the XdmaBackend ring-buffer feature, CR-008).

Status: TESTS PASSED (from TESTS REVIEWED)

## Requirements

Aspect: new optional JMAP section `dmaChannels`.

- The JMAP format gains an optional top-level section `dmaChannels`, parallel
to `addressSpace` and `interruptHandler`.
- `dmaChannels` is an object keyed by string keys representing non-negative
integer channel indices. Keys must be plain decimal digit strings; negative
values, keys with a leading sign (e.g. "-1" and "+1"), non-numeric strings,
and effective duplicates (different string representations of the same
numeric value, e.g. "1" and "01") cause a `ChimeraTK::logic_error`.
- Each entry is an object that must contain a key `type` holding a string. All
other keys of an entry are backend-specific and are not interpreted by the
generic parser.
- A file without `dmaChannels` parses unchanged.
- The channel indices are the same as the `channel` field of `address` objects
of type `"DMA"` in the `addressSpace`. The generic parser does not validate
this cross-reference, as backends may support standard DMA channels without
`dmaChannels` configuration.

Aspect: preservation of the section.

- The generic parser preserves each `dmaChannels` entry as raw `nlohmann::json`
and keeps it verbatim; no backend-specific structure is defined in the
generic `NumericAddressedRegisterCatalogue`.
- The parser rejects a shallowly malformed section: `dmaChannels` not an
object, a key that is not a parseable non-negative integer, an entry that is
not an object, or an entry without a string-valued `type`. This is a
`ChimeraTK::logic_error`, wrapped with the established
"Error parsing JSON map file ..." prefix.

Aspect: access from backends.

- The preserved entries are reached through the register catalogue, which
backend-specific subclasses already hold as the protected member
`_registerMap` (`NumericAddressedBackend.h`). No getter is added to
`NumericAddressedBackend` itself.
- Two public query methods on the catalogue: `hasDmaChannel(uint64_t index)
const` (a `std::map` lookup, no allocation, no throwing) and
`getDmaChannel(uint64_t index) const` (returning a const reference to the raw
`nlohmann::json` object, throws `ChimeraTK::logic_error` if the index is not
configured).
Note: The catalogue is read-only after construction, so concurrent access is safe.
- The `type` value determines which backend interprets an entry; a backend
will throw a `ChimeraTK::logic_error` if it finds a `type` it does not
support.

## Specifications

Affected components: `src/JsonMapFileParser.cc`,
`include/NumericAddressedRegisterCatalogue.h` /
`src/NumericAddressedRegisterCatalogue.cc`, `doc/jmapFormat.dox`,
`schemas/jmap.schema.json`, jmap fixtures and parser tests.

Aspect: parser.

- In `JsonMapFileParser::Imp::parse()` (`src/JsonMapFileParser.cc:598`),
parse `dmaChannels` as an `nlohmann::json` object alongside `addressSpace`
and `interruptHandler`. Validate the shallow envelope; a malformed envelope
throws inside the existing `try` block so the established error prefix is
applied. Entries are stored as-is, no fallback defaults. Each key is parsed
with `std::from_chars` into `uint64_t` (as for `mapFormatVersion`), requiring
the whole string to be consumed as plain decimal digits, so "-1", "+1",
non-numeric and overflowing keys are rejected; a key whose numeric value is
already present under a different spelling (e.g. "1" and "01") is rejected
as an effective duplicate.
- Absence of `dmaChannels` leaves the catalogue member empty; the existence
test then reports an absent channel and the getter throws. Presence is
stored through the catalogue's `setDmaChannel` setter (see the register
catalogue aspect).

Aspect: register catalogue.

- `NumericAddressedRegisterCatalogue` gains a protected member holding the
channel entries: `std::map<uint64_t, nlohmann::json> _dmaChannels` (default
empty). Because the parser fills this member and is not a subclass of the
catalogue, the catalogue also gains a public setter
`void setDmaChannel(uint64_t index, nlohmann::json entry)`, mirroring how the
existing public `addDataConsistencyRealm`/`addRegister` populate the other
protected members; the parser calls it once per validated `dmaChannels`
entry.
- Two public query methods are added to `NumericAddressedRegisterCatalogue`:
`bool hasDmaChannel(uint64_t index) const` (non-throwing, O(log n) lookup) and
`const nlohmann::json& getDmaChannel(uint64_t index) const` (returns a const
reference to the raw json object, throws `ChimeraTK::logic_error` if the index
is not configured). These allow backend-specific subclasses to access the
preserved channel entries.
- `clone()`/`fillFromThis()` (`src/NumericAddressedRegisterCatalogue.cc:320`)
copy the new member like the existing `_listOfInterrupts`,
`_canonicalInterrupts` and `_dataConsistencyRealms`.

Aspect: documentation.

- `doc/jmapFormat.dox` documents `dmaChannels` in a new section: a minimal
explanation that the generic parser validates only the shallow envelope
(object, non-negative integer keys, object entries with a string `type`) and
preserves each entry verbatim, and that the entry content is interpreted
solely by the consuming backend. No example and no reference to a specific
backend or to later extensions are given yet. The top-level key list in the
"Fundamental concepts" overview gains `dmaChannels`.
- `schemas/jmap.schema.json` gains a `dmaChannels` top-level property so the
strict schema (top-level `additionalProperties: false`) stays consistent with
the documented format, mirroring `interruptHandler`: the value is an object
whose keys match a non-negative-decimal pattern, and each value is an object
with a required string `type` and otherwise unrestricted backend-specific
keys (`additionalProperties: true` inside the entry object, unlike the
strict `interruptHandlerEntry`), because the entry content is opaque to the
generic schema.

## Test plan

All new parser tests go into the existing source file
`tests/executables_src/testJsonMapFileParser.cpp`; no additional jmap files are
added.

- Parser test: the valid `dmaChannels` section is added to `simpleJsonFile.jmap`,
the jmap file used in the existing parser tests. The entries reach the by-index
getter with their `type`.
- Parser test: a jmap file without a `dmaChannels` section parses, e.g.
`bitRangeChannels.jmap`. The existence test reports an absent channel and the
by-index getter throws.
- Parser test: `hasDmaChannel` reports a present index as found; the by-index
getter returns the entry.
- Parser test: the entry's backend-specific keys beyond `type` (including
nested objects and arrays) are preserved verbatim in the raw json returned by
the by-index getter.
- Parser test: the by-index getter on an absent index throws a
`ChimeraTK::logic_error`.
- Parser test: each malformed envelope (`dmaChannels` not an object, non-integer
key, non-object entry, missing `type`, non-string `type`) throws
`ChimeraTK::logic_error`. The key-validation cases
cover negative keys (e.g. "-1"), keys with a leading sign (e.g. "+1"),
non-numeric keys (e.g. "hello"), an empty key (e.g. "") as a non-numeric
key, values exceeding `uint64_t` (e.g. "18446744073709551616"), and
effective duplicates (e.g. "1" and "01"), each throwing
`ChimeraTK::logic_error`. As in the existing fault
tests, the broken jmap file is generated by loading `simpleJsonFile.jmap` with
`nlohmann::json`, modifying the `dmaChannels` section, writing it to a
temporary file which is parsed in the test and deleted afterwards.
- Catalogue test: cloning a catalogue preserves the channel entries (via
`clone()).`

## Deferred issue

- doc/jmapFormat.dox deviates from the change request specification: the spec (Specifications, documentation aspect) explicitly requires 'No example and no reference to a specific backend or to later extensions are given yet', but the implemented section contains both a specific-backend reference (line 315: 'the first consumer is the XdmaBackend ring-buffer feature') and an example verbatim block (lines 318-323 with 'Xdma' entries). The documentation must be brought in line with the specification (drop the example and the specific-backend reference).
