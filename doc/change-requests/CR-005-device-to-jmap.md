# CR-005: Tool to write a jmap file from a device catalogue

Synopsis: Command-line tool `chimeratk-device-to-jmap` opens a device
(via ChimeraTK device descriptor or dmap file plus alias), reads its
register catalogue through the generic Device interface, and writes a
jmap file. A DummyBackend opened with that file exposes a matching
register catalogue.

Status: DONE

## Requirements

Aspect: command-line tool generating jmap files from a live device
catalogue.

- Shipped as C++ executable `chimeratk-device-to-jmap`, built and
  installed to `bin` (cf. existing tools in `CMakeLists.txt`).
- Works with any backend accessible via the generic Device interface.
  Primarily targets non-`NumericAddressed` backends
  (e.g., `LogicalNameMapping`). Does not rely on numeric addressing.
  - Using it on a `NumericAddressed` backend is pointless: such a
     backend already has a usable map file.
- Source device identified by a single command-line argument: a
  ChimeraTK device descriptor (CDD) or an alias name.
  - The optional `--dmap` provides a dmap file path for alias
     resolution; it never changes the interpretation of `--device`.
- Opens the device, reads its register catalogue
  (`Device::getRegisterCatalogue()`) and writes a jmap file in format
  version `"0.0.1"` (see `doc/jmapFormat.dox`).
- A `ChimeraTK::logic_error` thrown while opening the device is caught;
  the tool reports the error to stderr and exits with status 1 instead
  of terminating via an unhandled exception.
- Uses only the generic Device interface and `RegisterInfo`
  (`include/RegisterInfo.h`).
- Generated jmap matches source catalogue except for lossy
  transformations:
  - Two-dimensional registers omitted with warning.
  - Writable registers with `AccessMode::wait_for_new_data` lose
     writability (warning emitted).
  - Addresses are synthetic and ignored in comparisons.
- Remaining registers match in: register paths, number of elements,
  access rights (modulo above), fundamental data type, signedness,
  integrality, element size (ASCII strings use the fixed
  `--string-length` element size, not the source).
- Output is deterministic: identical input yields identical jmap file.
- Output file given by a single mandatory `--output` option; the tool
  never writes to stdout.
- Optional `--string-length <bytes>` sets the fixed byte size per
  element assumed for ASCII strings; defaults to 80. The generic
  Device interface does not expose the byte size of strings.

Aspect: representation of generic register information in the jmap.

- Registers with `AccessMode::wait_for_new_data` written as
  interrupts: top-level `triggeredByInterrupt` entry, read-only.
  Writable variants emit a single stderr warning listing all affected
  registers.
- Supported representations:
  - Strings as ASCII, void registers as `representation` type `void`.
     Strings always use a fixed length of `--string-length` bytes per
     element.
  - String arrays supported, all elements sharing the common fixed
     `bytesPerElement` from `--string-length`.
- Two-dimensional registers representable but deliberately
  unsupported: treated as unsupported, skipped.
- Non-integral numeric payloads assumed IEEE754; fixed-point
  undetectable generically.
  - Fixed-point registers are practically absent for
     non-`NumericAddressed` backends, the tool's primary target.
- Integral numeric registers written as `fixedPoint` with zero
  fractional bits, matching byte size and signedness.
- `width`, `isSigned` and `bytesPerElement` derived from generic data
  descriptor via `DataDescriptor::minimumDataType()`, except for ASCII
  strings, which use `--string-length` (default 80) bytes per element.
  - Numeric-addressed specifics (width, fractional bits, element
     size) are never copied from a `NumericAddressedBackend`.
- Boolean registers: 4-byte fixed-point with `width=1`. Bitmask
  packing rejected due to low count, simplicity, and performance
  considerations.
- Unsupported registers (two-dimensional) trigger single stderr
  warning listing all affected registers, skipped. Tool continues
  with partial jmap.

## Specifications

Aspect: affected components.

- New C++ source file: `tools/src/chimeratk-device-to-jmap.cc`
  (separate from library sources in `src/`). File layout: `main()`
  first, all helper functions after it, with forward declarations and a
  comment for each at the top of the file; no anonymous namespace.
- `CMakeLists.txt`: new executable target
  `chimeratk-device-to-jmap`, linked against project library,
  installed to `bin`.
- Installed `chimeratk-device-to-jmap` must find its shared library
  when installed into a user defined prefix: set `INSTALL_RPATH` on
  the executable to the install library directory (e.g.
  `$ORIGIN/../${CMAKE_INSTALL_LIBDIR}`). Without it the tool fails
  with "cannot open shared object file" for
  `libChimeraTK-DeviceAccess.so` on installs outside the system
  default search path.
- No library API changes.

Aspect: command line interface.

- Parsing via `boost::program_options` (add `program_options` to
  Boost components in `CMakeLists.txt`). Options:
  - Mandatory: `--device <device>`: CDD or alias name.
  - Optional: `--dmap <dmap file>`: sets the dmap file path for
     alias resolution.
  - Mandatory: `--output <file>`: path of the jmap file to write.
     The tool never writes to stdout.
  - Optional: `--string-length <bytes>`: fixed byte size per element
     assumed for ASCII string registers; default 80.
- `--help` prints usage and limitations:
  - Two-dimensional registers skipped (single warning listing all
     affected registers).
  - Writable registers with `AccessMode::wait_for_new_data` become
     read-only (single warning listing all affected registers).
  - Synthetic addresses in output.
  - ASCII strings written with a fixed length of `--string-length`
     bytes (default 80).

Aspect: opening the device.

- `--device` handling never depends on `--dmap`: it is always passed
  to `Device::open()` as given.
- When `--dmap` is given, it only sets the dmap file path via
  `BackendFactory::getInstance().setDMapFilePath(...)`.
- `BackendFactory::createBackend()` resolves both CDDs and aliases, so
  a CDD works with or without `--dmap`.
  - With a CDD, `--dmap` has no effect.
- `device.open()` may throw a `ChimeraTK::logic_error` (e.g. for an
  unknown backend or an invalid descriptor); the call is wrapped in a
  handler that reports the error to stderr and exits with status 1,
  instead of terminating via an unhandled exception.

Aspect: catalogue reading and jmap generation.

- Sorts registers alphabetically by full path using `std::ranges::sort`,
  iterates
  `Device::getRegisterCatalogue()`. One jmap `addressSpace` entry per
  register, mirrors `RegisterPath` hierarchy with `children` objects
  for modules.
- A path component may be both a register and a module (sub-registers
  below it); register data and the `children` object then coexist in
  one entry. `insertRegister` handles leaf and module nodes uniformly:
  the destination object is created if absent, a pre-existing object
  is extended in place (setting the `children` key promotes a missing
  node to an object; the old contents are never copied into a newly
  built object), and at the leaf the register fields are merged in
  asserting that no existing key is overwritten (an overwrite would
  indicate an internal inconsistency, e.g. a duplicate register path).
  An existing object is never silently replaced.
- No exception escapes `main`: the catalogue dump is wrapped in a
  handler that reports a `std::logic_error` (thrown by `insertRegister`
  on an internal inconsistency) to stderr and exits with a non-zero
  status, instead of terminating via an unhandled exception.
- Per register: `access` (`RW`/`RO`/`WO` from `isReadable()`/
  `isWriteable()`), `numberOfElements`, `representation` from generic
  `DataDescriptor`.
- `bytesPerElement`: `minimumDataType().getNumberOfBytes()` for
  numeric, 4 for boolean, `--string-length` for strings, 0 for void.
- Registers with `AccessMode::wait_for_new_data`: top-level
  `triggeredByInterrupt`, one interrupt per register, no `access`
  field (parser rejects both).
- JSON via `nlohmann::json` (existing dependency). Serializer
  configured for determinism: sorted keys, consistent float precision,
  and pretty printing (indented with two spaces) so the output is
  human-readable.
- Address assignment: contiguous blocks on bar 0 in alphabetical
  order, padded to multiples of 4 bytes (dummy's minimum transfer
  alignment) to prevent overlaps. Void registers written without
  address.

Aspect: unsupported registers.

- Trigger stderr warning listing all affected register paths, exclude
  from output. Generation never aborted.

## Test plan

- New boost test executable: `tests/executables_src/testDeviceToJmap.cc`.
- Round trips:
  - Via numeric-addressed device: open `DummyBackend` on
     `tests/simpleJsonFile.jmap`, dump jmap from catalogue, open
     `DummyBackend` on generated file, compare catalogues using
     generic `RegisterInfo` accessors.
  - Via non-numeric device: open `LogicalNameMapping` backend on
     `tests/valid.xlmap`, dump jmap, open `DummyBackend` on it,
     compare catalogues. Proves tool independence from numeric
     addressing.
    - The fixtures are non-overlapping: `simpleJsonFile.jmap` covers
        the full feature set, `valid.xlmap` the non-numeric path.
- Catalogue comparison implemented as a helper
  `compareRegisters(const RegisterInfo&, const RegisterInfo&)` that
  holds both comparison rules: access rights modulo the lossy
  writable-to-read-only interrupt transformation, and
  signedness/integrality/element size only for numeric registers.
  `compareCatalogues` runs on the two `RegisterCatalogue` objects
  directly, pairing register paths via `hasRegister()` and
  `getRegister()`, instead of converting both into a `std::map` of
  extracted register properties. The writability guard is simplified
  with De Morgan's theorem to
  `source.isWriteable() != destination.isWriteable() &&
  (!source.isWriteable() || destination.isWriteable())`.
- Individual property assertions use the `RegisterCatalogue`
  directly: presence is checked with a single `hasRegister()` call
  per path, properties are read via `getRegister()`. No
  `requireRegister` helper or catalogue map is used; a tiny
  helper that only wraps a `hasRegister` check is not worth having.
- Test helper functions are kept only where they avoid code
  duplication (e.g. `runTool`, `readFile`, `cleanup`,
  `openGenerated`, `generateFromDummy`/`generateFromLMap`), not for
  one-line wrappers.
- `LogicalNameMapping` variables are writable and support
  `AccessMode::wait_for_new_data`. Exercise requirement that writable
  registers with `wait_for_new_data` become read-only interrupts with
  single warning.
  - A `NumericAddressed` fixture cannot provide this case: the parser
     rejects both `access` and `triggeredByInterrupt`.
- Coverage mapping. Features exercised by concrete backends/registers
  listed below:

| Feature | Backend / fixture | Register(s) |
| --- | --- | --- |
| String (ASCII, fixed `bytesPerElement`) | LNM | `var_string` |
| Numeric integral types | LNM | `var_int8` ... `var_uint64` |
| IEEE754 payload | LNM | `var_float32`, `var_float64` |
| Boolean (`width` 1) | Dummy | `ENA`, `INACTIVE_BUF_ID` (in `DOUBLE_BUF`) |
| `wait_for_new_data` interrupt (1D) | LNM | all variables |
| Void pure interrupt | Dummy | `VOID_INTERRUPT_0`, `VOID_INTERRUPT_3_0_1` |
| Writable `wait_for_new_data` read-only (single warning listing all variables) | LNM | all variables |
| Constant | LNM | `Constant`, `Constant2`, `ArrayConstant` |
| Access `RW`/`RO`/`WO` | LNM, Dummy | `var_int32`, `Constant`, `SomeTable` |
| Array register | LNM | `ArrayVariable`, `ArrayConstant` |
| Module hierarchy | LNM | `MyModule.SomeSubmodule.Variable` |
| Redirected register / channel / bit | LNM | `SingleWord` ... `Bit3ofVar` |
| Plugin behaviour | LNM | `SingleWord_Scaled` ... `CustomUnitDescription` |
| 2D register skipped, with warning | Dummy | `DAQ.CTRL` ... `COLLISION.FD` |
| `selectedBy` muxed register | Dummy | `MUX` ... `DAQ.SINGLE_MUXED_ALT` |
| Edge case: empty catalogue | Dummy (empty jmap) | n/a |
| Edge case: all unsupported | Dummy (2D-only jmap) | 2D parents skipped |

`LNM` = `LogicalNameMapping` on `tests/valid.xlmap`. `Dummy` =
`DummyBackend` on `tests/simpleJsonFile.jmap`, unless the fixture is
named in the row.

- The 2D-only source fixture is built by loading the existing fixture
  `tests/muxedDataAccessor.jmap` via `nlohmann::json` and stripping its
  `addressSpace` down to the single two-dimensional register `TEST/DMA`.

- `Access`: `RW`/`RO`/`WO` verified on `SomeTopLevelRegister` (`RW`),
  `Constant` and `BSP/VERSION` (`RO`) and `APP/SomeTable` (`WO`). LNM
  `wait_for_new_data` variables are written read-only, so they cannot
  verify `RW`.

- Identification modes: `--device` with CDD and `--device` with
  `--dmap` plus alias produce same jmap for equivalent devices.
- Every invocation passes `--output`; running without `--output` exits
  with an error and writes nothing to stdout.
- `--help` prints usage text and exits without requiring a device.
- Determinism: consecutive dumps of same catalogue are byte-identical.
- `--string-length`: accepted with a default of 80; string registers
  are written with the configured byte size per element.
- Warning validation: for each warning exactly one warning is emitted
  and every affected register is listed; unsupported registers are
  omitted from output and a valid partial jmap is produced.
- Device open failure: invoking `--device` with a device that cannot
  be opened (e.g. an unknown backend or invalid descriptor) exits with
  status 1 and reports the `ChimeraTK::logic_error` message on stderr.
