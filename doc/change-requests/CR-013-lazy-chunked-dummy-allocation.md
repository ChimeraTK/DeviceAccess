# CR-013: Lazy chunked allocation of the dummy backends' address space

Synopsis: The dummy backends allocate each bar's full address space up front,
which is slow for map files with huge, mostly unused address spaces. This change
allocates the address space lazily in fixed-size byte chunks, gives each
materialised SharedDummyBackend chunk its own shared-memory object, and reworks
the backdoor accessors accordingly.

Status: DONE

## Requirements

- Each bar's address space is no longer fully allocated at construction
(DummyBackend) resp. first open (SharedDummyBackend), but lazily in chunks.
- The chunk size is a `constexpr size_t` constant; no other code assumes its
value.
- The `bar` and `address` parameters of `read`/`write` keep their meaning;
transfers are stitched transparently across chunk boundaries.
- Reading a never-allocated chunk yields zeros.
- SharedDummyBackend no longer reserves the full map address space in a single
shared segment. Each materialised chunk becomes its own shared-memory object,
created on demand.
- The materialised chunks are tracked by a chain, so they can be reset and
removed without bounding their number.
- Processes running incompatible library versions must never share a
segment. The layout version is part of the segment name, so an incompatible
version derives a different name and cannot attach, in either direction.
- A map file with changed content under the same name must not crash or raise
confusing errors, even when a bar size has grown.
- There is no restriction on the address range a bar may cover; any address is
valid and materialises chunks on demand.
- The internal storage type of the bar contents changes from `int32_t` to
`std::byte`.
- The backdoor accessors no longer expose references or pointers into the
internal buffers. They return value-semantic proxies which handle the chunk
discontinuities and support arbitrary byte-aligned registers (multi-byte
access via `memcpy`).
- `DummyBackend` keeps a deprecated library-side compatibility shim for legacy
downstream code that indexes the internal bar contents directly as
`_barContents[bar][wordIndex]`. The shim is a protected member, so legacy
subclasses inherit access to it, as they did before. The member keeps the name
`_barContents` (the new chunk map is renamed `_barChunks`) and its
square-bracket operator hands out a raw backdoor accessor for an entire bar, so
the old indexing expressions keep compiling. The member declaration and its
square-bracket operator are marked `[[deprecated]]`, so legacy code using it
compiles with warnings; the library's own declaration of the member stays
warning-free.
SharedDummyBackend exposes no shim: its former `_barContents` chunk map is only
renamed `_barChunks`.
- Read-only registers from the map file are still rejected on write, but
enforcement stays entirely in the accessor layer.
- The SharedDummyBackend interrupt storage keeps its current bounded layout and
is not chunked.

## Specifications

### Chunked bar storage

- The chunk map (renamed `_barChunks`, freeing the name `_barContents` for the
compatibility shim) becomes a flat, byte-based map keyed by `(bar, chunkIndex)`.
The value is `std::map<std::pair<uint64_t,uint64_t>, std::vector<std::byte>>`
for DummyBackend and `std::map<std::pair<uint64_t,uint64_t>,
SharedMemoryByteVector*>` for SharedDummyBackend, where
`SharedMemoryByteVector = boost::interprocess::vector<std::byte,
ShmemByteAllocator>`.
- `ShmemAllocator` stays an `int32_t` allocator for the `PidSet`; a separate
byte allocator type is introduced for the chunk vectors.
- A new `constexpr size_t CHUNK_SIZE` (1 MiB) in `DummyBackendBase` drives all
chunk-index arithmetic.
- Chunks are created zero-initialised on first access; reads materialise missing
chunks too. Only the touched chunk is materialised.
- `_barChunks` stays in the derived classes. Their value types differ, and
`DummyBackendBase` is in the core library and cannot use the
boost-interprocess types.
- Each backend implements one virtual storage method returning a `std::byte*` to
a chunk, creating it zero-initialised on first access in its own map and
allocator. It is the only method that touches the map.
- A transfer helper in `DummyBackendBase` maps `(bar, byteOffset, size)` onto
chunk indices, stitches the transfer across chunk boundaries and calls the
virtual storage method for each touched chunk. It only ever handles
`std::byte*`, so the two storage types stay hidden in the derived classes. It
is used by:
- `DummyBackend::read`/`write`;
- `SharedDummyBackend::read`/`write`;
- `writeRegisterWithoutCallback` (DummyBackend only);
- the backdoor accessors.
- The transfer helper takes no lock; the caller holds the backend's storage
lock:
- `DummyBackend::read`/`write`/`writeRegisterWithoutCallback` hold
`DummyBackend::mutex`, as today;
- `SharedDummyBackend::read`/`write` hold the interprocess mutex;
- each backdoor element access takes `DummyBackend::mutex` itself.
- Every `(bar, address)` access is valid, even outside any map-defined range.
- `getBarSizesInBytesFromRegisterMapping()` and the per-bar sizes derived from
it are removed, together with the sizing they served.
- The method is part of the installed `DummyBackendBase` API; its removal is
an accepted API break for downstream subclasses.
- It was the only construction-time `elementPitchBits % 8` check at the
DummyBackend level; the check in
`NumericAddressedBackendRegisterAccessor` covers the accessor path.

### SharedDummyBackend shared memory

- Layout: one small fixed-size home segment plus one shared-memory object per
materialised chunk.
- Chunk objects are named `<base>_BAR_<bar>_CHUNK_<index>` and are always
removed by their full derived name.
- Home segment contents:
- the pid set;
- the unchanged interrupt storage (`ShmForSems`:
`semEntries[SHARED_MEMORY_N_MAX_MEMBER]` and
`interruptEntries[maxInterruptEntries]`);
- the head and tail of a singly linked list of the chunk segments, which
together reference every materialised chunk.
- The home segment size is computed from these by the existing
`getRequiredMemoryWithOverhead()`, now reduced to the pid set, the chunk list,
the interrupt storage and their allocator overhead, and independent of the map
file. The following are removed:
- `getTotalRegisterSizeInBytes()`;
- the now unused `SHARED_MEMORY_CONST_OVERHEAD`;
- the now unused `SHARED_MEMORY_OVERHEAD_PER_VECTOR`.
- Names and versioning:
- The whole name is derived by one public static helper,
`SharedDummyBackend::getSharedMemoryName`. It replaces and absorbs
`Utilities::createShmName`, which is removed: constructing the shared-memory
name does not belong in a general-purpose utility.
- The helper builds the base name from the instance-id hash, the map file name
and the user, then appends the layout version,
`<base>_v<SHARED_MEMORY_LAYOUT_VERSION>`.
- `SHARED_MEMORY_LAYOUT_VERSION` is a `static constexpr`, bumped on every
shared-memory layout change.
- Everything derives from this base, so two library versions never attach to
each other's objects, in either direction:
- the home segment;
- the interprocess mutex (base name, as today);
- every chunk segment (`<base>_BAR_<bar>_CHUNK_<index>`).
- The helper is used by the constructor, the stale-lock recovery path and the
tests, so they cannot diverge. The tests that built the name through
`Utilities::createShmName` directly are updated to use it (see test plan).
- The previously unused `RequiredVersion` object is removed.
- Chunk chain:
- Each chunk segment carries a small fixed header with its `(bar, chunkIndex)`
key and the name of the next segment, empty for the last one.
- The chain is in creation order, appended at the tail. It is not sorted by
bar or address, so a successor may belong to any bar or range.
- The home segment's tail is a hint for O(1) appends. Appending first walks
to the true tail, so a tail left behind by a crash is harmless.
- It is only walked to reset or remove chunks, never to look one up, which
always goes by the derived name; the order therefore does not matter.
- It stores names, not mapped pointers, because mapped addresses differ
between processes.
- Appending a materialised `(bar, chunkIndex)`, under the already-held
interprocess mutex:
1. Find the true tail: start at the home segment's tail if it names an
existing segment, otherwise at the head, and follow the successor links
through existing segments to the last one. A successor whose segment does
not exist ends the walk; the chain is empty when neither the tail nor the
head names an existing segment.
1. Set that tail's successor (for an empty chain, the head) to the derived
chunk name.
1. Open or create the segment and construct its vector.
1. Advance the home segment's tail to the derived chunk name.
- This order makes the append crash-safe:
- Walking to the true tail first recovers from a previous crash that happened
before the tail was advanced: the walk reaches the segment that was created
but not yet made the tail, so the append cannot overwrite the link to it.
Normally the tail is already the true tail and the walk is a single step.
- The link is written before the segment exists, so a created segment is
always reachable from the head and cannot leak; the tail is advanced only
after the segment exists, so it never names a missing segment.
- A walker reaching a name whose segment does not exist treats it as the end of
the chain and so misses nothing; prepending would hide the existing chain
behind the missing segment.
- If step 3 fails, the early link is rolled back before the error is thrown.
- Chunks are never relocated or remapped once created, so cached
`SharedMemoryByteVector*` pointers stay valid.
- There is no per-chunk refcount and no upper bound on the number of chunks;
`/dev/shm` capacity bounds the touched working set, not the map size.
- Crash recovery, when `checkPidSetConsistency()` finds only dead pids:
`reInitMemory()` walks the chain and removes or resets the chunk segments,
instead of `listNamedElements()`, which only ever saw the home segment.
`setupBarContents()` only resets the per-process chunk maps.
- Teardown, when the last process leaves, under the mutex:
1. Save the head, then clear the head and tail, so a racing process sees an
empty chain.
1. Remove the chunk segments by walking the saved chain.
1. Remove the home segment.
A racing process therefore rebuilds, instead of following a chain whose
segments are being removed.
- Every chain walk that removes or resets segments (recovery and teardown)
tolerates a link whose segment does not exist: such a name is skipped, not
mapped with `open_only`. A process killed between writing a link and creating
its segment leaves exactly such a dangling link, and mapping it would throw a
boost `interprocess_exception`: in recovery it would escape the
`SharedMemoryManager` constructor, so a later reopen after the crash would
fail with a confusing error, and in teardown it would reach the
`std::terminate()` handler and kill an unrelated healthy process on exit.
Skipping keeps both paths silent and stops the crash from violating the
"must not crash or raise confusing errors" guarantee.
- If creating a chunk fails (`std::bad_alloc` or a boost
`interprocess_exception`, e.g. no space or no free inode in the shared-memory
filesystem), the operation throws `ChimeraTK::runtime_error` with a
descriptive message. This replaces the construction-time
`ChimeraTK::logic_error` thrown by `setupBarContents()` on `bad_alloc`.

### Map file independence

- The map file does not limit the valid range of any bar. Chunk vectors are
always `CHUNK_SIZE` big, so a process joining a segment created with a
different map content can neither index out of range nor overrun; stale chunks
of a formerly larger map stay unused.

### Callbacks and read-only

- `_writeCallbackFunctions` stays address/range based and keeps its per-address
semantics. `AddressRange::sizeInBytes` becomes 64-bit so a span of arbitrary
size is never truncated. The `AddressRange` layout change is an accepted ABI
break; the header is installed.
- The `DummyBackend` read-only members are removed:
- `setReadOnly`;
- the `_readOnlyAddresses` set;
- `isReadOnly()`.
- `isWriteRangeOverlap()` then collapses to a pure overlap test.
- Read-only registers from the map file are enforced at the accessor layer
(`NumericAddressedBackendRegisterAccessor` throws `logic_error` on a write to
a non-writeable register). A read-only write through the normal Device
interface never reaches the backend, so the backend-level skip is dead in
practice.
- `writeRegisterWithoutCallback` continues to bypass any read-only protection,
as it must for resync inside write callbacks, and performs no read-only
filtering.
- Accepted consequences:
- A direct `DummyBackend::write` (a public method used by tests) to a
read-only mapped address now succeeds.
- Reads or writes beyond the map-defined bar size no longer throw
`logic_error` (any address is valid).
- The `out_of_range` handling in `TRY_REGISTER_ACCESS` becomes unused.
- A map file with a non-byte-aligned top-level register no longer throws when
the backend is constructed or opened; the `elementPitchBits % 8` check now
runs only when an accessor is built.

### Backdoor accessors

- The backdoor accessors and their proxies store `(backend, bar, byteOffset)`
instead of a raw pointer. The affected accessors are:
- `DummyRegisterAccessor`;
- `DummyMultiplexedRegisterAccessor`;
- `DummyRegisterRawAccessor`.
- `proxies::RawByteAccess` holds `(backend, bar, byteOffset)` and performs an
element's byte read/write through the transfer helper. Its `read`/`write` are
defined inline in the header (`DummyRegisterAccessor.h`), not in the `.cc`
file, to keep the per-element access overhead low.
- An element access copies exactly the element's bytes (its `RawType` size,
which equals the element width) via the transfer helper (`memcpy`), keeping
the `std::byte*` interface of `RawConverterCapsule`. The storage access is
byte-granular, so neighbouring bytes are never modified.
- `DummyRegisterRawAccessor` becomes a class template over the raw type,
`DummyRegisterRawAccessor<RawType = int32_t>`. A deduction guide is provided
for the 32-bit case. The `DummyBackend::getRawAccessor` factory is itself a
template, `template<typename RawType = int32_t> DummyRegisterRawAccessor<RawType>
getRawAccessor(...)`, so a non-default raw type is obtainable through it, with
`int32_t` as the default.
- The deduction guide resolves the raw type to `int32_t`, so a deduction-less
`DummyRegisterRawAccessor acc{...}` uses 32-bit elements, like
`getRawAccessor`; another width needs an explicit template argument.
- The default template argument does not itself enable a deduction-less
declaration at an existing use site: class template argument deduction is
not applied in a non-static data member initialiser, so a member such as
`testNumericAddressedBackendUnified.cpp:225`
(`DummyRegisterRawAccessor acc{exceptionDummy, ...}`) must name the default
explicitly as `DummyRegisterRawAccessor<int32_t> acc{...}`. This is part of the
test adaptation (see test plan); the deduction guide still serves ordinary
variable declarations.
- A value-semantic proxy cannot be passed directly to `std::bit_cast`:
`std::bit_cast` deduces the proxy type, not `RawType`, and no implicit
conversion applies. A read that feeds `std::bit_cast` converts the proxy to
`RawType` first; the affected tests are adapted accordingly (see test plan).
- It returns value semantics instead of `int32_t&`; its proxy converts to and
from `RawType` and implements, so that existing expressions (`raw += 5`,
`raw++`, `raw & mask`, `~raw`) keep compiling and update the memory:
- the compound-assignment operators (`+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`,
`^=`, `<<=`, `>>=`);
- pre/post-increment/decrement;
- the binary operators `operator&` and `operator~`;
- an implicit conversion to `RawType`.
- Conversions between the proxy and `RawType` use direct-initialisation
(`RawType(value)`) rather than `static_cast<RawType>(value)`, for readability;
the added tests follow the same form.
- `sizeof(RawType)` must equal the element width (`elementPitchBits / 8`); an
assert catches a wrong instantiation.
- The 32-bit alignment restrictions are replaced by byte offsets. The affected
code is:
- the `address % sizeof(int32_t)` check in
`DummyRegisterAccessor::getElement`;
- the `elementPitchBits % (8 * sizeof(int32_t))` check in the same method;
- the word-strided indexing in
`DummyMultiplexedRegisterAccessor::operator[]`;
- the word-strided indexing in `proxies::DummyRegisterSequence`.
- The `elementPitchBits % 8 == 0` constraint remains, as sub-byte pitches are
not supported.
- Byte offsets and sizes are 64-bit throughout:
- `_offsets`;
- `_nbytes`;
- `_pitch`;
- `DummyRegisterElement::_nbytes`.
- `DummyRegisterAccessor::setWriteCallback` no longer truncates `bar` to
`uint8_t` or `address` to `uint32_t`, so slabs above 4 GiB are not truncated.
- The backdoor accessors exist only on `DummyBackend` and take a `DummyBackend&`
argument; `SharedDummyBackend` derives from `DummyBackendBase` and has none.
Each element access takes `DummyBackend::mutex`, materialises the chunk
through the transfer helper and copies the element bytes. All
SharedDummyBackend materialisation goes through its own `read`/`write` under
the interprocess mutex.
- The backdoor accessors no longer expose pointers into the buffers, so the
buffer lock has lost its purpose: `getBufferLock()` is removed from
`DummyRegisterAccessor` and `DummyRegisterRawAccessor`. This is an accepted
API break. Tests that held it drop the explicit locking, and the short
per-element lock scope also removes the lock-order inversion with the math
plugin conversion.

### Deprecated bar-contents compatibility shim

- For legacy downstream code that indexes the internal bar contents directly,
`DummyBackend` exposes a protected compatibility member named `_barContents`;
the actual chunk storage lives in `_barChunks` (see "Chunked bar storage").
Its member is `protected`, as the historical member was, so legacy subclasses
inherit it. `SharedDummyBackend` has no shim: its former `_barContents` chunk
map is renamed `_barChunks` and no compatibility member is added, because the
historical member was `private` and no downstream code could reach it.
- The member is of a new compatibility type. The member declaration and its
square-bracket operator are marked `[[deprecated]]`, so any use in legacy code
produces a compiler deprecation warning. The type itself is not deprecated:
decorating the type would make `DummyBackend`'s own member declaration a use of
a deprecated entity and warn in the library build. The library itself never
uses the shim, so the library builds warning-free.
- `_barContents[bar]` hands out the `DummyBackend` raw backdoor accessor for
the entire bar (the same value-semantic `DummyRegisterRawAccessor` used by the
backdoor accessors), so the shim shares its type and semantics.
`DummyRegisterRawAccessor` is otherwise built from a named register, taking its
bar, address, element pitch and element count from the register map entry; a
whole bar, which must be valid even outside any mapped range, has no such
entry. The shim therefore constructs it register-lessly from `(backend, bar,
byte offset 0)` with a fixed 32-bit element. Its `operator[](wordIndex)`
selects the `int32_t` word at byte offset `wordIndex * sizeof(int32_t)`,
reproducing the historical word-based layout of the former
`std::map<uint64_t, std::vector<int32_t>>`. The mapping from word index to byte
offset is the deprecated legacy layout; new code uses byte offsets through the
backdoor accessors.
- The accessor is value-semantic like the other backdoor accessors: an element
read/write goes through the transfer helper and so materialises chunks and
copies exactly the element's bytes. `memcpy` keeps the access byte-granular.
- Bar bounds: any bar is valid, consistent with the "any address is valid"
rule; the shim bounds-checks nothing.
- Locking: each element access takes `DummyBackend::mutex`, exactly like the
other backdoor accessors. The shim exists only on `DummyBackend`, the only
backend with backdoor accessors and hence the only one with this storage path.

### Alternatives considered

- Keep `int32_t` as internal storage type: rejected, keeps the reinterpret_casts
and blocks byte-aligned accessors.
- Chunk only SharedDummyBackend and keep full per-bar allocation for
DummyBackend: rejected, although the plain DummyBackend does not really suffer
the construction cost, chunking both keeps them in sync and avoids supporting
two storage models in the shared transfer helper and the backdoor accessors.
- Reserve the full map address space in the shared segment, or key the segment
name by a map content hash: rejected, the shared memory filesystem (e.g. a 64
MiB `/dev/shm` in containers) is often too small and creating the segment
would be slow.
- Grow a single shared segment on demand via `managed_shared_memory::grow`:
rejected, Boost.Interprocess only offers off-line growing (every process must
unmap first) and remaps the segment, invalidating the raw pointers which
address the shared chunks.
- Track the materialised chunks in a fixed registry in the home segment:
rejected, a `managed_shared_memory` segment cannot grow, so the registry would
impose an artificial reserved upper bound; the linked chain is unbounded.
- Discover the chunk segments by scanning `/dev/shm` for the name prefix:
rejected, Boost.Interprocess has no cross-segment enumeration and it would add
a Linux-only filesystem dependency.
- Create the chunk segment first and link it afterwards: rejected, a crash in
between leaks the segment, which is not yet reachable from the chain and whose
name no process knows.
- Chunk the interrupt storage as well: rejected, it is already bounded
(`maxInterruptEntries`, `SHARED_MEMORY_N_MAX_MEMBER`) and fails loudly when
full, and the dispatcher caches pointers into the array; a larger limit is a
constant change.

## Test plan

- Tests check functionality only, no timing or performance assertions.
- Tests that need a special map file write a minimal map file in jmap format to
a temporary file and load it from there, instead of adding another file to the
shared list of test map files.
- Test-file organisation: each new case is added to the existing test file of
the entity under test; no new test executables are introduced. The chunked
allocation cases go into `testDummyBackend`, the chunked shared-memory cases
into `testSharedDummyBackend` (and its Ext variants) and the backdoor-proxy
cases into `testDummyRegisterAccessor`.
- Chunk boundaries: read/write of a register crossing a chunk boundary is
correct; neighbouring chunks stay zero.
- Reading an untouched chunk returns zeros and materialises exactly that one
chunk, consistent with the specification ("reads materialise missing chunks
too"); a write materialises only the touched chunk.
- Address access beyond any map-defined range works; reads yield zeros and
writes materialise chunks on demand.
- SharedDummyBackend:
- only used address ranges exist in shared memory; multi-process access still
works (existing shared dummy tests);
- a minimal jmap file written to a temporary file with an unreasonably huge
address space (e.g. a 1000 TB register) opens and works while accessing only
the first and the last few bytes; full reservation would exhaust any shared
memory, so succeeding proves that nothing is reserved up front and that only
the touched chunks materialise as shared objects;
- a process using a map file with the same name but increased bar sizes joins
an existing segment without crash or confusing errors;
- the chunk chain is walked correctly, and after the last process leaves
(also after a killed process is detected) no chunk segment of the instance
is left in shared memory; a write to a chunk far beyond the map-defined
range followed by close leaves no chunk segment behind, since the last
process leaving removes every segment; the test is named (for example,
`testCleanupRemovesChunkBeyondMapRange`) and commented for this cleanup
behaviour, not for cross-instance sharing, which the last process leaving
cannot exhibit (reopening removes the segments) and which the existing forked
`testJoinSegmentWithIncreasedBarSizes` already covers;
- a link naming a non-existent segment (a process killed between writing the
link and creating its segment) ends a chain walk silently and does not crash
cleanup: teardown removes the still-existing chunks and leaves nothing behind;
- processes built with different layout versions use different segment names
and never attach to each other's segment.
- Backdoor accessors:
- byte-aligned registers (odd byte offsets, misaligned pitch) work; registers
spanning a chunk boundary work;
- existing accessor behaviour in `testDummyRegisterAccessor` is preserved;
- the raw accessor works with value semantics and still compiles and updates
the memory for:
- the compound-assignment operators;
- inc-dec;
- the binary operators `&` and `~`;
- conversion to the underlying type;
- the raw accessor instantiated for a non-default `RawType` (e.g. `int16_t`)
accesses the memory correctly.
- Deprecated compatibility shim: exercised on a `DummyBackend` through a
subclass, so the inherited protected access is covered;
`_barContents[bar][wordIndex]` compiles and reads/writes the `int32_t` word at
byte offset `wordIndex * sizeof(int32_t)`, with the deprecation warning
suppressed around the legacy expression. `SharedDummyBackend` has no shim;
`_barChunks` is private there, as the historical member was, so no test
accesses it directly and its tests observe the shared memory from outside by
counting chunk segments.
- Adapt the tests to the new behaviour:
- obsolete read-only tests (e.g. `testDummyBackend`'s `testReadOnly`) are
removed rather than adapted: the dummy backend no longer has a read-only
feature, which is enforced solely in the `NumericAddressedBackend` accessor
layer, so the tests go together with the removed `setReadOnly`,
`_readOnlyAddresses` and `isReadOnly` members;
- `DummyRegisterTest::testRegisterAccessor` in `testDummyRegisterAccessor` is
rewritten against the public API (ordinary accessors) instead of the backdoor
accessor, which it currently exercises against itself;
- tests that assert sizes or full reservation (`testDummyBackend`,
`testDummyRegisterAccessor`) assert lazily materialised content only and use
the renamed chunk map `_barChunks`;
- tests that write the internal bar contents directly (`testDummyBackend`,
`testDummyRegisterAccessor`) may keep the legacy `_barContents[bar][wordIndex]`
indexing through the deprecated compatibility shim, suppressing the deprecation
warning, or go through the backdoor accessors; the legacy indexing is available
only on `DummyBackend` (the listed `testDummyBackend` and
`testDummyRegisterAccessor` are `DummyBackend`-based), while
`SharedDummyBackend` tests have neither path: `_barChunks` is private there, as
the historical member was, so they observe the shared memory from outside by
counting chunk segments;
- the out-of-range `logic_error` expectations follow the new "any address is
valid" behaviour.
- the tests that derived the shared-memory name themselves obtain it through
`SharedDummyBackend::getSharedMemoryName(...)` instead of the removed
`Utilities::createShmName(...)`:
- `testMapFileSearch`, which used `Utilities::createShmName(...)` directly;
- `testSharedDummyBackendExt`;
- `testSharedDummyBackendUnified` and its Ext variant;
- the layout-versioning test, which folds into `testSharedDummyBackendExt`;
- `tests/scripts/testSharedDummyBackendPidManagement.sh` runs
`testSharedDummyBackendExt` and needs no change of its own.
- Adapt the other backdoor and `AddressRange` consumers for the new accessor
semantics:
- `testGenericMuxedInterruptDistributor`;
- `testNumericAddressedBackendRegisterAccessor`: the
`std::bit_cast<float>(<raw element>)` read-backs wrap the raw element in
`RawType(...)`;
- `testNumericAddressedBackendUnified`, where the member declaration at line
225 needs the explicit `DummyRegisterRawAccessor<int32_t>`, since no
class template argument deduction applies in a non-static data member
initialiser;
- `testLMapBackendUnified`;
- `testDoubleBufferAccessor`.
