# CR-012: Backend-specific interrupt handler entries in the JMAP file

Synopsis: The JSON map parser hardcodes an "INTC" controller into the interrupt
handler metadata and silently drops any other content. Make the interrupt
handler metadata a pass-through applying only a common normalisation (drop
underscore-prefixed keys, default a missing version) so backend-specific
controller types (e.g. "MyINTC") with arbitrary data are preserved.

Status: DONE

## Requirements

Aspect: interrupt handler metadata is backend agnostic.

- The parser must not interpret the content of an interrupt handler entry
beyond a common normalisation applied to every entry: keys starting with an
underscore `_` (e.g. `_comment`) are dropped, and a missing `version` member
is set to 1. The normalised entry is passed on as metadata: all remaining
members, keys and values are preserved, including the backend-specific
controller type key (e.g. "INTC" or "MyINTC") and its data structure. The
metadata value equals the normalised entry re-serialised with `json::dump()`
(object keys sorted).
- Metadata keys and subhandler recursion are unchanged: key `!<intId>`, nested
`subhandler` entries append their id components.
- A map carrying a backend-specific controller type with data incompatible with
"INTC" parses and reaches the metadata, normalised per the common rules.
- Existing "INTC" maps continue to parse; their metadata value becomes the
normalised, re-serialised entry (underscore keys dropped, missing `version`
defaulted, nothing else changed) instead of today's deeper normalisation
(all defaults filled, comments dropped).

## Specifications

Aspect: parser (`src/JsonMapFileParser.cc`).

- Keep `InterruptHandlerEntry` as the recursion carrier; its `subhandler` map
and `fill()` already recurse with the id appended.
- Drop the typed `Controller` struct and the fixed `INTC` member (currently
lines 566-593); the fixed member hardcodes "INTC" and silently drops unknown
keys, so a backend-specific entry would be replaced by an empty default
"INTC" controller.
- Give `InterruptHandlerEntry` a raw `nlohmann::json` member holding the
entry minus its `subhandler` member, and replace the
`NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT` macro with a custom
`from_json` that performs this split, applies the common normalisation and
deserialises `subhandler` recursively.
- The common normalisation is applied in `from_json` before storing the raw
member: keys starting with `_` are erased and a missing `version` is set to
1. `fill()` emits the normalised raw member's `dump()`.
- The emitted value is produced by `json::dump()`, so object keys are sorted;
the existing `![3]` value stays byte-identical.
- `schemas/jmap.schema.json` is left unchanged; it is only meant for
interactive validation and is not enforced by the parser.
- CR-006's `mapFormatVersion` enforcement is already merged in this branch's
base; CR-012's changes coexist with it, and all test maps carry
`mapFormatVersion: "1.0"`.

Aspect: documentation (`doc/jmapFormat.dox`).

- The `\ref jmap_interrupts` section states that the controller entry content
is backend-specific and extensible.
- It states that the JSON schema is advisory only (interactive validation) and
is not enforced by the parser.

## Test plan

All parser tests in `tests/executables_src/testJsonMapFileParser.cpp`.

- Regression test: an interrupt handler entry `MyINTC` carrying an arbitrary,
INTC-incompatible data structure (nested object, unexpected value types) is
parsed; the metadata values `![<id>]` equal the entries normalised (underscore
keys dropped, `version` defaulted to 1) and re-serialised with `json::dump()`
without `subhandler`. This fails on the current parser.
- `TestGoodMapFileParse` expectations: `![3,0]` and `![3,1]` revert to their
original normalised strings (`"version":1` defaulted again); `![3]` stays
unchanged; `![0]` becomes `{"INTC":{"path":"DAQ","version":1}}` - only the
`options` member that the old parser defaulted is no longer emitted.
