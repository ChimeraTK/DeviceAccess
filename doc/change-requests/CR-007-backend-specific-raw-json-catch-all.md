# CR-007: Raw-json DMA channel configuration in the JMAP file

Synopsis: Add an optional, opaque `dmaChannels` top-level section to the JMAP
file format, preserved as raw `nlohmann::json` and exposed to backend-specific
subclasses of `NumericAddressedBackend`. The generic parser validates only the
shallow envelope; the content is interpreted by the specific backend (first
consumer: the XdmaBackend ring-buffer feature, CR-008).

Status: PLANNED

## Requirements

Aspect: new optional JMAP section `dmaChannels`.

- The JMAP format gains an optional top-level section `dmaChannels`, parallel
  to `addressSpace` and `interruptHandler`.
- `dmaChannels` is an object keyed by string keys representing non-negative
  integer channel indices. Keys must be parseable as `size_t` values; negative
  values, non-numeric strings, and effective duplicates (different string
  representations of the same numeric value, e.g. "1" and "01") cause a
  `ChimeraTK::logic_error`.
- Each entry is an object that must contain a key `type` holding a string. All
  other keys of an entry are backend-specific and are not interpreted by the
  generic parser.
- A file without `dmaChannels` parses unchanged.

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
- Two public query methods on the catalogue: `hasDmaChannel(size_t index)
  const` (a `std::map` lookup, no allocation, no throwing) and
  `getDmaChannel(size_t index) const` (returning a const reference to the raw
  `nlohmann::json` object, throws `ChimeraTK::logic_error` for an absent index).
- The `type` value determines which backend interprets an entry; a backend
  will throw a `ChimeraTK::logic_error` if it finds a `type` it does not
  support.

## Specifications

Affected components: `src/JsonMapFileParser.cc`,
`include/NumericAddressedRegisterCatalogue.h` /
`src/NumericAddressedRegisterCatalogue.cc`, `doc/jmapFormat.dox`, jmap
fixtures and parser tests.

Aspect: parser.

- In `JsonMapFileParser::Imp::parse()` (`src/JsonMapFileParser.cc:572`),
  parse `dmaChannels` as an `nlohmann::json` object alongside `addressSpace`
  and `interruptHandler`. Validate the shallow envelope; a malformed envelope
  throws inside the existing `try` block so the established error prefix is
  applied. Entries are stored as-is, no fallback defaults.
- Absence of `dmaChannels` leaves the catalogue member empty; the existence
  test then reports an absent channel and the getter throws.

Aspect: register catalogue.

- `NumericAddressedRegisterCatalogue` gains a protected member holding the
  channel entries: `std::map<size_t, nlohmann::json> _dmaChannels` (default
  empty).
- Two public query methods are added to `NumericAddressedRegisterCatalogue`:
  `bool hasDmaChannel(size_t index) const` (non-throwing, O(log n) lookup) and
  `const nlohmann::json& getDmaChannel(size_t index) const` (returns a const
  reference to the raw json object, throws `ChimeraTK::logic_error` if index
  not found). These allow
  backend-specific subclasses to access the preserved channel entries.
- `clone()`/`fillFromThis()` (`src/NumericAddressedRegisterCatalogue.cc:320`)
  copy the new member like the existing `_listOfInterrupts`,
  `_canonicalInterrupts` and `_dataConsistencyRealms`.

Aspect: documentation.

- `doc/jmapFormat.dox` documents `dmaChannels` in a new section: the envelope
  rules, the mandatory `type` field, and that the rest is backend-specific. The
  section explains that channel indices are non-negative integers and the
  content of each entry is interpreted solely by the consuming backend.

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
- Parser test: each malformed envelope (`dmaChannels` not an object, non-integer
  key, non-object entry, missing `type`, non-string `type`) throws
  `ChimeraTK::logic_error` with the map-file prefix. The key-validation cases
  cover negative keys (e.g. "-1"), keys with a leading sign (e.g. "+1"),
  non-numeric keys (e.g. "hello"), values exceeding `size_t` (e.g.
  "18446744073709551616"), and effective duplicates (e.g. "1" and "01"), each
  throwing `ChimeraTK::logic_error`. As in the existing fault
  tests, the broken jmap file is generated by loading `simpleJsonFile.jmap` with
  `nlohmann::json`, modifying the `dmaChannels` section, writing it to a
  temporary file which is parsed in the test and deleted afterwards.
- Catalogue test: cloning a catalogue preserves the channel entries (via
  `clone()`).
