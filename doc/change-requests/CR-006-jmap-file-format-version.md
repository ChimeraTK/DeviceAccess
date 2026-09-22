# CR-006: JMAP file format version 1.0

Synopsis: Change the JMAP map file format version from the three-component
string `"0.0.1"` to the two-component string `"1.0"` (MAJOR.MINOR, no patch
component) and make the JMAP parser enforce exactly this version, throwing a
`ChimeraTK::logic_error` on any mismatch. The version change does not imply a
change in the file format itself; it only declares that the format is settled
and moving towards a production release.

Status: DONE

## Requirements

Aspect: JMAP file format version.

- The JMAP file format version is a `MAJOR.MINOR` version string without a
  patch component; the format release this change settles on carries the
  version `"1.0"`.
- The previous format version `"0.0.1"`, which contains a patch component, is
  no longer a valid JMAP format version.

Aspect: version enforcement in the parser.

- The JMAP parser must read the `mapFormatVersion` entry and require its major
  and minor components to be `1` and `0`: the string must consist of exactly
  two dot-separated components, each a non-empty sequence of decimal digits,
  and the components are compared numerically rather than as strings, so
  `"1.0"`, `"1.00"` and `"01.0"` are equivalent.
- Any other value throws a `ChimeraTK::logic_error`. This includes the old
  `"0.0.1"`, other versions such as `"1.1"` or `"2.0"`, a version carrying a
  patch component (e.g. `"1.0.1"`), a malformed version string, and a file
  without a `mapFormatVersion` entry.
- No backwards compatibility with old `"0.0.1"` files is provided; such files
  now fail to parse. This behaviour is accepted.

Aspect: consistency of declared and documented versions.

- Every place that declares, generates or documents the JMAP format version
  must use `"1.0"`: the test fixtures, the map-to-jmap converter, the
  documentation, the JSON schema, and the device-to-jmap tool introduced by
  CR-005.

## Specifications

Affected components: `src/JsonMapFileParser.cc`, the parser test suite
`tests/executables_src/testJsonMapFileParser.cpp`, the test `.jmap`
fixtures under `tests/`, `schemas/jmap.schema.json`,
`doc/jmapFormat.dox`, `tools/chimeratk-map-to-jmap`, and the new tool
`chimeratk-device-to-jmap` and its test suite
`tests/executables_src/testDeviceToJmap.cc`, both introduced by CR-005.

Aspect: parser version check.

- `JsonMapFileParser::Imp::parse()` (in `src/JsonMapFileParser.cc`) currently
  reads `addressSpace`, `metadata` and `interruptHandler` but ignores
  `mapFormatVersion`. It gains a version check directly after `json::parse`,
  inside the existing `try` block so that the error is wrapped with the `"Error
  parsing JSON map file '<name>': "` prefix already established there.
- A supported map format version is defined as a constant holding the major
  component `1` and the minor component `0`, kept next to the parser.
- The value of `mapFormatVersion` is parsed with `std::from_chars` into one
  `uint32_t` per component, in two inline steps: the major component is parsed
  from the start of the string and must be directly followed by exactly one
  `'.'`; the minor component is then parsed from after that `'.'` up to the
  end of the string. Parsing fails (throwing a `ChimeraTK::logic_error`) when
  the major component is absent or overflows `uint32_t`, when it is not
  followed by a `'.'`, when the minor component is absent, overflows
  `uint32_t` or is followed by a trailing character, or when either parsed
  component differs numerically from the supported major (1) or minor (0).
  Because the comparison is numeric, `"1.0"`, `"1.00"`, `"01.0"` and
  `"01.00"` are all accepted, while a version with a patch component
  (`"1.0.1"` or the old `"0.0.1"`), a single component (`"1"`), a component
  that is not a pure digit sequence (e.g. `"1.0 "` or `"1.x"`), a component
  whose digit sequence is too large for the `uint32_t` comparison, or a
  mismatched major or minor (`"0.0"`, `"1.1"`, `"2.0"`) is rejected.
- Any failure - a missing `mapFormatVersion` key, a wrongly typed value, the
  wrong number of components, a non-decimal component, or a major/minor
  mismatch - is surfaced as a `ChimeraTK::logic_error`. The version check runs
  inside the existing `try` block: a missing key or a wrong value type first
  raises an `nlohmann::json` exception, which the existing outer `catch`
  converts, together with any directly thrown `ChimeraTK::logic_error`, into a
  `ChimeraTK::logic_error` prefixed with
  `` "Error parsing JSON map file '<name>': " ``. `std::from_chars` reports a
  malformed or oversized component through its error code rather than by
  throwing, so no `std::out_of_range` or similar exception escapes from the
  version check itself.

Aspect: fixtures, converter and documentation.

- Set `mapFormatVersion` to `"1.0"` in all test fixtures:
  `tests/simpleJsonFile.jmap`, `tests/simpleJsonFile.diffBar.jmap`,
  `tests/muxedPolled.jmap`, `tests/muxedDataAccessor.jmap`,
  `tests/bitRangeChannels.jmap`, `tests/bitRangeChildCases.jmap`,
  `tests/unifiedTest.jmap` and `tests/selectedByInheritance.jmap`. The
  fixtures keep their content otherwise identical.
- Set the emitted `mapFormatVersion` to `"1.0"` in
  `tools/chimeratk-map-to-jmap`.
- Update the two `"0.0.1"` references in `doc/jmapFormat.dox` to `"1.0"`.
- Update `mapFormatVersion` in `schemas/jmap.schema.json`: its description
  must reference `"1.0"` and its pattern must allow exactly two
  dot-separated components (matching MAJOR.MINOR with no patch component).
- Update the version string emitted by the `chimeratk-device-to-jmap` tool
  created in CR-005 to `"1.0"`, so that files generated by that tool pass the
  new parser check.
- Update the hardcoded `"0.0.1"` in the inline empty-source fixture of
  `TestEmptyCatalogue` in `tests/executables_src/testDeviceToJmap.cc` to
  `"1.0"`, so that opening that fixture with the new parser check succeeds.
  The 2D-only source fixture in the same file copies `mapFormatVersion` from
  `tests/muxedDataAccessor.jmap` and therefore inherits the `"1.0"` from the
  fixture update; it needs no change.

## Test plan

- The existing parser tests keep passing with the updated fixtures: a fixture
  declaring `"1.0"` parses without error.
- A single new test case in `tests/executables_src/testJsonMapFileParser.cpp`
  covers all `mapFormatVersion` checks. It throws `ChimeraTK::logic_error`
  for the old `"0.0.1"` (a three-component version, which contains a patch
  component; this check subsumes the previously separate `"1.0.1"` case),
  `"1.1"`, `"2.0"`, `"1"` (single component), an absent major or minor
  component (e.g. `".0"` or `"1."`), a major component not followed by a
  dot (e.g. `"1x.0"`), a non-decimal minor component (e.g. `"1.0 "`), a
  component whose digit sequence is too large for the numeric comparison
  (e.g. `"1.999999999999999999999999"` for the minor, or
  `"999999999999999999999999.0"` for the major), an absent entry, and a
  wrongly typed entry, and it accepts `"1.0"` as well as the numerically
  equivalent forms `"1.00"`, `"01.0"` and `"01.00"`, verifying the
  numeric rather than string comparison. The version can be varied by
  injecting it into a base fixture with `nlohmann::json`, using the same
  helper approach as the existing injected-selectedBy tests.
