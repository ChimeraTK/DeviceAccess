# CR-013: Lazy chunked allocation of the dummy backends' address space

Synopsis: The dummy backends allocate each bar's full address space up front,
which is slow for map files with huge, mostly unused address spaces. This change
allocates the address space lazily in fixed-size byte chunks, gives each
materialised SharedDummyBackend chunk its own shared-memory object, and reworks
the backdoor accessors accordingly.

Status: TESTS REVIEWED

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
- Read-only registers from the map file are still rejected on write, but
enforcement stays entirely in the accessor layer.
- The SharedDummyBackend interrupt storage keeps its current bounded layout and
is not chunked.

## Specifications

### Chunked bar storage

- `_barContents` becomes a flat, byte-based map keyed by `(bar, chunkIndex)`.
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
- `_barContents` stays in the derived classes. Their value types differ, and
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
`testDummyRegisterAccessor`) assert lazily materialised content only;
- tests that write the internal bar contents directly (`testDummyBackend`,
`testDummyRegisterAccessor`) go through the backdoor accessors instead, with
no library-side compatibility shim;
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

## Deferred issue

- DI-1 [DONE] Test-plan contradiction with the versioned shared-memory name: the spec appends `_v<SHARED_MEMORY_LAYOUT_VERSION>` to `Utilities::createShmName(...)` (CR-013 lines 92-105), but the test plan claims `testSharedDummyBackendExt`, `testSharedDummyBackendUnified`/Ext and the pid-management script "still pass" (lines 298-303). Those tests build `shmName` from the unversioned `Utilities::createShmName(...)` (tests/unitTestsNotUnderCtest/testSharedDummyBackendExt.cpp:37, testSharedDum...
- DI-2 [DONE] Undefined locking contract for chunk materialisation, with a deadlock risk against `getBufferLock()`: the spec says the backdoor accessors materialise chunks "under the DummyBackend mutex" and the shared helper is used by `DummyBackend::read`/`write`, which already hold `DummyBackend::mutex` (backends/DummyBackend/src/DummyBackend.cc:51,63). `getBufferLock()` returns a lock on that same non-recursive mutex (backends/DummyBackend/include/DummyRegisterAccessor.h:200, :396) and...
- DI-3 [DONE] The single shared transfer helper cannot reach the two differently-typed chunk maps: after the change `_barContents` stays in the derived classes with different value types (`std::vector<std::byte>` in DummyBackend.h:103 vs `SharedMemoryByteVector*` in SharedDummyBackend.h:70), and `DummyBackendBase` (core lib) cannot know the boost-interprocess types. The spec (lines 59-64) has one helper in the base that "is the only place that materialises chunks" and is used by both back...
- DI-4 [DONE] Minor wording: the spec (line 240) lists `writeRegisterWithoutCallback` as a SharedDummyBackend materialisation path, but it is a `DummyBackend`-only method (backends/DummyBackend/include/DummyBackend.h:124); `SharedDummyBackend` has none and only materialises through its own `read`/`write`. Correct the wording.
- DI-5 [DONE] Minor: removing `getBarSizesInBytesFromRegisterMapping()` (src/DummyBackendBase.cc:72-82) also removes the only construction-time `elementPitchBits % 8` check, so a map file with a non-byte-aligned top-level register no longer throws when opened (it only fails when an accessor is built). The requirement says the constraint "remains" and the "Accepted consequences" list (lines 188-193) does not record this behaviour change; confirm it is intended and add it there.
- DI-6 [DONE] Test plan omits tests/executables_src/testLMapBackendUnified.cpp from all adaptation lists (lines 338-357). It holds the removed buffer lock at lines 153 and 175 (`derived->acc.getBufferLock()`), exactly like testNumericAddressedBackendUnified (lines 917/938) which IS listed, and uses `DummyRegisterAccessor` extensively. With `getBufferLock()` removed and each element access now taking `DummyBackend::mutex` itself (CR lines 262-267, 256-261), this file will not compile / will ...
- DI-7 [DONE] testDummyBackendUnified.cc is listed under "Adapt the other backdoor and AddressRange consumers" (line 357) but uses none of the affected APIs: it contains only an unused `#include "DummyRegisterAccessor.h"`, no backdoor accessor, `AddressRange`, `setReadOnly`, `getBufferLock` or `_barContents`, and the `UnifiedBackendTest` framework it calls does not use them either. The entry is unjustified (minor).
- DI-8 [DONE] CR-013 test plan (line 330) asserts the existing raw-accessor expressions in testNumericAddressedBackendRegisterAccessor 'still compile','but they cannot, and the fix is mutually exclusive with the CR metadata: the test deduces the raw type from `acc & derived->bitmask` (int32_t, line 227) and reads via `bit_cast<float>(acc)` (lines 375/393/411). CR lines 225-228 reject a default-attributed deduction guide (DI-2 already rejected the resulting ambiguity), so `DummyRegisterRaw...
- DI-9 [DONE] Undecided in the CR: for a deduction-less `DummyRegisterRawAccessor acc{...}` (e.g. testNumericAddressedBackendUnified.cpp:225), what fixed width should the accessor use, given CR line 238 requires `sizeof(RawType)` to equal the element width (`elementPitchBits / 8`) and CR lines 225-228 reject a default-attributed (and hence ambiguous) deduction guide?
- DI-10 [DONE] Undecided in the CR: should the raw proxy expose a bit_cast-compatible conversion to the register's 32-bit value (testNumericAddressedBackendRegisterAccessor.cpp:375/393/411 do `bit_cast<float>(acc)`), or is that read-back expression part of the test adaptation to be dropped/rewritten in the tests-write phase?
- DI-11 [DONE] testDummyRegisterAccessor.cpp:210-257 writes int32_t words directly into `_barContents[0xD]` in the pre-change contiguous bar layout (offset areaIndexOffset = 1000/pitch); with chunked byte storage only the backdoor proxy can place them correctly, confirming the CR-mandated adaptation (test plan line 339) - needs confirmation that no library-side compatibility shim is expected.
- DI-12 [DONE] Raw-accessor type/conversion contract unrecorded (DI-8/DI-9/DI-10): the CR endorses a deduction guide (lines 225-228) but never states that it resolves to DummyRegisterRawAccessor<int32_t>, nor that a value-semantic proxy cannot be passed to std::bit_cast (deduction picks the proxy type, not RawType, so the implicit conversion does not apply); testNumericAddressedBackendRegisterAccessor.cpp:375/393/411 bit_cast<float>(<raw element>) therefore cannot compile unchanged, and t...
- DI-13 [DONE] Test plan does not state the raw read-back adaptation (DI-8/DI-10): the entry for testNumericAddressedBackendRegisterAccessor (line 354) does not mention that the bit_cast<float>(<raw element>) read-backs must wrap the proxy in RawType(...).
- DI-14 [DONE] Test plan does not record that tests writing _barContents directly are rewritten through the accessors (DI-11): testDummyRegisterAccessor.cpp:210-257 (and testDummyBackend.cpp) write int32_t words straight into _barContents, which no longer exists as a per-bar contiguous word buffer; state that these go through the backdoor accessor with no library-side compatibility shim.
- DI-15 [DONE] Genuine implementation defect (not a plan problem): after the refactor of the fixed-point proxy, proxies::DummyRegisterElement<T> stores _rc and _nbytes as VALUE members (backends/DummyBackend/include/DummyRegisterAccessor.h:114 and :117). The derived DummyRegisterAccessor<T> still declares its OWN _rc member (:239) and its constructor does `this->_rc = _rc; this->_nbytes = ...` (:192-193), which now assign to the DERIVED member only. At the merge base (10388c0a) the base s...
- DI-16 [DONE] Reproduced: building tests/testDummyRegisterAccessor and running it from its CTest working dir (build/.../tests) aborts with 'fatal error ... memory access violation at address: 0x0: no mapping at fault address' at DummyRegister test suite/DummyRegisterTest__testRegisterAccessor; the crash occurs on the direct `device->someRegister = 3` / `device->someRegister == 3` expressions (tests/executables_src/testDummyRegisterAccessor.cpp:113-114), i.e. through the base operator= / ...
- DI-17 [DONE] Suggested fix (implementation phase): in DummyRegisterAccessor<T>, stop shadowing the base members - remove the derived `_rc` (:239) and the redundant `_nbytes` handling, and construct/materialise the capsule so the base owns it (e.g. pass the made capsule into the base in the member-initialiser list and let the base own _rc/_nbytes, as at the merge base), or restore the base _rc to a reference bound to the derived member. Add a regression test on the direct DummyRegisterAc...
- DI-18 [DONE] Blocker for the tests-write phase: the CR-mandated adaptations themselves are done and the 5 affected test files compile (tests/executables_src/testDummyBackend.cpp, testDummyRegisterAccessor.cpp, testLMapBackendUnified.cpp, testNumericAddressedBackendUnified.cpp, testNumericAddressedBackendRegisterAccessor.cpp), but testDummyRegisterAccessor cannot pass while the library defect above is present, so the phase cannot be completed as specified.
- DI-19 [DONE] tests/core: a 117 MB ELF core dump is committed to the branch (added in commit e32c969b, tracked by git and not covered by .gitignore). It is an accidental binary artifact, not part of the change request; remove it from the change (and ignore core dumps) before the change can proceed.
- DI-20 [DONE] backends/DummyBackend/include/DummyBackend.h:19-21: the newly added forward-declaration block `namespace proxies { class RawByteAccess; }` is not closed with a `// namespace proxies` comment; the project style requires a closing comment (clang-tidy llvm-namespace-comment).
- DI-21 [DONE] tests/executables_src/testChunkedBackdoorAccessor.cpp:18: `#include <memory>` is unused (no std smart-pointer use; boost::make_shared comes from boost/make_shared.hpp). Reported by clang-tidy misc-include-cleaner on a newly added line.
- DI-22 [DONE] tests/executables_src/testChunkedSharedDummyBackend.cpp:24: `#include <vector>` is unused (the file uses SharedMemoryByteVector, not std::vector). Reported by clang-tidy misc-include-cleaner on a newly added line.
- DI-23 [DONE] tests/executables_src/testDummyBackend.cpp:571: the added check `dummyBackend->_barContents.size() != 0` should use `!dummyBackend->_barContents.empty()` (clang-tidy readability-container-size-empty).
- DI-24 [DONE] clang-tidy cppcoreguidelines-pro-type-reinterpret-cast on new lines without the project's usual NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) (11 such NOLINTs exist in the tree): backends/DummyBackend/include/DummyRegisterAccessor.h:371,377; backends/DummyBackend/src/DummyBackend.cc:55,69,84; backends/SharedDummy/src/SharedDummyBackend.cc:85,102.
- DI-25 [DONE] clang-tidy readability-identifier-naming: newly added private members backends/SharedDummy/include/SharedDummyBackend.h:159 (chunkList) and :165 (chunkSegments) lack the required '_' prefix.
- DI-26 [DONE] clang-tidy modernize-avoid-c-arrays on new raw char arrays without NOLINTNEXTLINE(modernize-avoid-c-arrays) (project uses this NOLINT elsewhere): backends/SharedDummy/include/SharedDummyBackend.h:96,102,103,107,110,113,116.
- DI-27 [DONE] clang-tidy performance-move-const-arg: backends/DummyBackend/include/DummyRegisterAccessor.h:73 std::move(access) has no effect (RawByteAccess is trivially copyable); remove std::move.
- DI-28 [DONE] clang-tidy hicpp-signed-bitwise on new code: backends/DummyBackend/include/DummyRegisterAccessor.h:390,391,392,393,394,411,412; tests/executables_src/testDummyBackend.cpp:142,189; tests/executables_src/testChunkedBackdoorAccessor.cpp:126,202.
- DI-29 [DONE] clang-tidy bugprone-implicit-widening-of-multiplication-result: backends/DummyBackend/include/DummyBackendBase.h:74 (1024 * 1024 widened to size_t) and tests/executables_src/testChunkedSharedDummyBackend.cpp:290.
- DI-30 [DONE] clang-tidy modernize-use-nodiscard: backends/DummyBackend/include/DummyRegisterAccessor.h:368 (get) and backends/SharedDummy/include/SharedDummyBackend.h:171 (getChunkSegmentName).
- DI-31 [DONE] clang-tidy llvm-prefer-static-over-anonymous-namespace for new test helper functions in anonymous namespaces: tests/executables_src/testChunkedBackdoorAccessor.cpp:30; tests/executables_src/testChunkedDummyBackend.cpp:29; tests/executables_src/testChunkedSharedDummyBackend.cpp:36,68,74,85,98.
- DI-32 [DONE] clang-tidy modernize-loop-convert: tests/executables_src/testDummyBackend.cpp:248,258.
- DI-33 [DONE] clang-tidy performance-unnecessary-value-param: tests/executables_src/testDummyRegisterAccessor.cpp:47 createInstance(std::string ...) passes by value; if the factory signature requires it, add the project's NOLINT as used for the other createInstance functions.
- DI-34 [DONE] backends/DummyBackend/include/DummyRegisterAccessor.h:370 (get) and :377 (set): the // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) is not immediately before the cast - the line-wrapped proxies::RawByteAccess(...) call puts the reinterpret_cast on the following physical line (:372 read(), :379 write()), so clang-tidy-22 still reports the CR-added lines DummyRegisterAccessor.h:372:17 and :379:18 'do not use reinterpret_cast [cppcoreguidelines-pro-type-reinterp...
- DI-35 [DONE] tests/executables_src/testChunkedSharedDummyBackend.cpp:213-236: testTwoInstancesShareMaterialisedChunks does not test what its name and header comment claim. No second instance is ever created/opened and the header comment (lines 220-222) states it 'create[s], write[s] and close[s] one instance and then reopen[s] ... and reads the previously materialised chunk back', but the body only writes, closes and asserts that all chunk segments were removed (line 233), so the claime...
- DI-36 [DONE] tests/executables_src/testChunkedDummyBackend.cpp:118: the leading comment 'Reading a never-allocated chunk yields zeros and does not itself materialise the storage' contradicts the specification ('reads materialise missing chunks too'), contradicts the test's own inner comment at line 130 and its assertion chunkCount(0) == 1U at line 131 (the read does materialise exactly one chunk). Correct the comment so it matches the tested behaviour.
- DI-37 [DONE] clang-tidy readability-qualified-auto: backends/DummyBackend/include/DummyRegisterAccessor.h:376 declares `auto* bytes = reinterpret_cast<const std::byte*>(&value);` in DummyRegisterRawElement::set(); the pointee is const, so declare it as `const auto*` (readability-qualified-auto is enabled and the check fires on this CR-added line).
- DI-38 [DONE] clang-tidy readability-uppercase-literal-suffix: tests/executables_src/testDummyBackend.cpp:143 and :191 use the lowercase literal suffix `1u` (`1u << 20` and `1u << 21`); the enabled check requires the uppercase form `1U`. The `// NOLINTNEXTLINE(hicpp-signed-bitwise)` placed just above them is now unnecessary for an unsigned shift and can be removed while fixing the suffix.
- DI-39 [DONE] clang-tidy cppcoreguidelines-pro-type-union-access on the newly added lines of tests/executables_src/testDummyRegisterAccessor.cpp:231, 258, 281, 370 and 392: the new memcpy round-trips added by the CR access the test's packing union (`mixedReg.raw[...]` / `mixedReg.cooked.r...`) and the enabled check requires a NOLINTNEXTLINE (this deliberate union is the point of the test) or a rewrite that avoids union member access.
- DI-40 [DONE] Stray empty statement: `backends/DummyBackend/src/DummyBackend.cc:188` has a lone `;` on its own line immediately after the `auto asyncDomain = boost::dynamic_pointer_cast<...>(_asyncDomainsContainer.getDomain(interruptNumber));` statement in `DummyBackend::triggerInterrupt` (added line). Dead/accidental empty statement; remove it.
- DI-41 [DONE] Duplicate/shadowed member: `backends/DummyBackend/include/DummyRegisterAccessor.h:478` re-declares `boost::shared_ptr<DummyBackend> _backend;` in `DummyRegisterRawAccessor`, shadowing the identical protected `_backend` already inherited from the base `DummyRegisterRawElement<RawType>` (declared at line 424). The derived class shadows the base member (both are initialised in the constructor and can diverge); remove the derived re-declaration and use the inherited member.
- DI-42 [DONE] tests/executables_src/testChunkedSharedDummyBackend.cpp:218 - test case is still named `testTwoInstancesShareMaterialisedChunks`, contradicting its own body (it creates only one instance, writes at `3 * CHUNK + 16` and asserts cleanup after close, never exercising two instances) and the test plan (CR-013 lines 343-346), which requires this test to be "named and commented for this cleanup behaviour, not for cross-instance sharing". The header comment (lines 214-217) was corr...
- DI-43 [DONE] SharedMemoryManager::removeAllChunks() (backends/SharedDummy/src/SharedMemoryManager.cc:316-335) walks the saved chunk chain and maps every name with managed_shared_memory(open_only, current) (line 324) without first checking that the segment exists. The append design deliberately permits a chain link that names a non-existent segment: appendToChain() writes the successor link before createChunk() creates the segment (SharedMemoryManager.cc:146-161, spec CR-013 lines 142-14...
- DI-44 [DONE] DummyRegisterRawAccessor is now a class template, but DummyBackend::getRawAccessor returns the fixed DummyRegisterRawAccessor<int32_t> (backends/DummyBackend/include/DummyBackend.h:80). The factory must itself become a template with RawType defaulting to int32_t, so a non-default raw type can be obtained through it; otherwise the templated accessor is unusable via getRawAccessor.
- DI-45 [DONE] Replace static_cast<T>(...) by T(...) in the CR-added code (same effect, easier to read), e.g. the proxy conversions proxies::DummyRegisterElement::operator T() which uses static_cast<T>(*this) (backends/DummyBackend/include/DummyRegisterAccessor.h, the CR-added lines around :227, :235, :243, :252) and the CR-added static_casts in the tests.
- DI-46 [DONE] RawByteAccess::read and RawByteAccess::write are performance-critical for some tests; move their implementations from backends/DummyBackend/src/DummyRegisterAccessor.cc:11-30 into the header (class proxies::RawByteAccess, backends/DummyBackend/include/DummyRegisterAccessor.h:48-66) and mark them inline.
- DI-47 [DONE] Obsolete tests must be removed, not adapted: the read-only tests (e.g. testDummyBackend's testReadOnly) test a feature the dummy backend no longer has - read-only enforcement now lives entirely in the NumericAddressedBackend accessor layer - so they should be deleted along with the removed setReadOnly/_readOnlyAddresses/isReadOnly members.
- DI-48 [DONE] Do not add new test .cpp files for cases of an entity that already has a test file: testChunkedDummyBackend.cpp, testChunkedSharedDummyBackend.cpp and testChunkedBackdoorAccessor.cpp duplicate existing entities and must be integrated into the existing files that already test the same entity (testDummyBackend, testSharedDummyBackend*/testSharedDummyBackendExt, testDummyRegisterAccessor).
- DI-49 [DONE] DummyRegisterTest::testRegisterAccessor in testDummyRegisterAccessor.cpp is pointless: it tests the backdoor register accessor against itself. Rewrite it against the public API (ordinary accessors) so it exercises a different layer.
- DI-50 [DONE] Shared-memory name derivation is duplicated: SharedDummyBackend::getSharedMemoryName() is just Utilities::createShmName(...) + "_v" + version (backends/SharedDummy/src/SharedMemoryManager.cc:50-53), yet Utilities::createShmName is still used directly in testMapFileSearch.cpp:197-198 and in testChunkedSharedDummyBackend.cpp:162. Remove Utilities::createShmName entirely and keep only getSharedMemoryName, since the name construction does not belong in Utilities; update the tes...
- DI-51 [DONE] clang-tidy on CR-added lines in tests/executables_src/testSharedDummyBackend.cpp: the chunked-test helpers are 'static' definitions inside the anonymous namespace (opened at line 31), which the project's clang-tidy flags with two enabled checks. Affected: line 60 'static constexpr uint64_t CHUNK' (readability-static-definition-in-anonymous-namespace), and lines 65 'writeJmap', 97 'chunkSegmentName', 103 'countChunkSegments', 116 'uniqueInstanceHash' (both readability-static...
- DI-52 [DONE] DI-52: tests artifact committed to the branch. `Testing/Temporary/LastTest.log` (a 3-line empty CTest run log) is tracked by git and was added by the CR-013 fixup commit b8ea2f3b; it is an accidental test-run artifact, not part of the change request. Remove it from the branch and add `Testing/` (and `core`, cf. DI-19) to the project-specific section of `.gitignore`.
- DI-53 [DONE] DI-53: clang-tidy readability-convert-member-functions-to-static on the five CR-added test methods in tests/executables_src/testDummyRegisterAccessor.cpp:507 (`testChunkedBackdoorByteAlignedRegister`), :545 (`testChunkedBackdoorRegisterSpanningChunkBoundary`), :568 (`testChunkedBackdoorRawAccessorValueSemantics`), :629 (`testChunkedBackdoorRawAccessorNonDefaultRawType`) and :660 (`testChunkedBackdoorRawAccessorDeductionGuide`). They are members of the `DummyRegisterTest` fi...
- DI-54 [DONE] clang-tidy-22 (misc-include-cleaner) on the CR-added line backends/SharedDummy/src/SharedMemoryManager.cc:8: `#include <boost/filesystem.hpp>` is reported 'included header filesystem.hpp is not used directly'. The header IS used (`boost::filesystem::path`/`boost::filesystem::absolute` at SharedMemoryManager.cc:55), so the check wants the directly used headers instead (`<boost/filesystem/path.hpp>` and `<boost/filesystem/operations.hpp>`). This is the only clang-tidy diagnos...
