# CR-012: Backend-specific interrupt handler entries in the JMAP file

Synopsis: The JSON map parser hardcodes an "INTC" controller into the interrupt
handler metadata and silently drops any other content. Make the interrupt
handler metadata a verbatim pass-through so backend-specific controller types
(e.g. "MyINTC") with arbitrary data are preserved.

Status: PLANNED

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

- Remove the fixed `InterruptHandlerEntry` and `Controller` structs
  (currently lines 566-593); they hardcode "INTC" and silently drop unknown
  keys, so a backend-specific entry would be replaced by an empty default
  "INTC" controller.
- Replace the deserialisation and `fill()` call in `parse()` (currently lines
  704-707) with generic JSON handling: a file-local recursive walk over the
  `interruptHandler` section that descends through `subhandler` members with
  the id appended, and for each entry with a non-empty id stores the key
  `"!" + json(id)` with the verbatim entry JSON minus its `subhandler` member
  as metadata value.
- The emitted value is produced by `json::dump()` of the entry, so object keys
  are sorted; the existing `![3]` value stays byte-identical.
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
