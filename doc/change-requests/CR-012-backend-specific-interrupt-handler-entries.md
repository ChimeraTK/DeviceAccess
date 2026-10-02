# CR-012: Backend-specific interrupt handler entries in the JMAP file

Synopsis: The JSON map parser hardcodes an "INTC" controller into the interrupt
handler metadata and silently drops any other content. Make the interrupt
handler metadata a verbatim pass-through so backend-specific controller types
(e.g. "MyINTC") with arbitrary data are preserved.

Status: IN PROGRESS (from PLANNED)

## Requirements

Aspect: interrupt handler metadata is backend agnostic.

- The parser must not interpret the content of an interrupt handler entry; the
entry is passed on verbatim as metadata, preserving the backend-specific
controller type key (e.g. "INTC" or "MyINTC") and its data structure.
- Metadata keys and subhandler recursion are unchanged: key `!<intId>`, nested
`subhandler` entries append their id components.
- A map carrying a backend-specific controller type with data incompatible with
"INTC" parses and reaches the metadata unchanged.
- Existing "INTC" maps continue to parse; their metadata value becomes the
verbatim entry instead of today's normalised form (defaults filled, comments
dropped).

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
`from_json` that performs this split and deserialises `subhandler`
recursively.
- `fill()` emits the raw member's `dump()` as the metadata value; the keys,
the recursion and the `parse()` call site are unchanged.
- The emitted value is produced by `json::dump()`, so object keys are sorted;
the existing `![3]` value stays byte-identical.
- `schemas/jmap.schema.json` is left unchanged; it is only meant for
interactive validation and is not enforced by the parser.

Aspect: documentation (`doc/jmapFormat.dox`).

- The `\ref jmap_interrupts` section states that the controller entry content
is backend-specific and extensible.
- It states that the JSON schema is advisory only (interactive validation) and
is not enforced by the parser.

## Test plan

All parser tests in `tests/executables_src/testJsonMapFileParser.cpp`.

- Regression test: an interrupt handler entry `MyINTC` carrying an arbitrary,
INTC-incompatible data structure (nested object, unexpected value types) is
parsed; the metadata value `![<id>]` equals the verbatim entry JSON without
`subhandler`. This fails on the current parser.
- Update the `TestGoodMapFileParse` metadata expectations for `![0]`, `![3,0]`
and `![3,1]` to the verbatim values (today's normalised strings); `![3]` is
unchanged.

## Deferred issue

- Undeclared dependency on CR-006 (JMAP version enforcement) which modifies the same parser and schema components assumed unchanged in CR-012
- Potential conflict between CR-012 stating schemas/jmap.schema.json is left unchanged and CR-006 having already modified it to enforce version 1.0
- Ambiguity whether CR-012's parser changes account for CR-006's stricter mapFormatVersion validation logic
- Test plan assumes pre-CR-006 baseline for metadata expectations ![0], ![3,0], ![3,1] which may not hold after CR-006's parser changes
