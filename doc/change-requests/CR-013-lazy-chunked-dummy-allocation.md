# CR-013: Lazy chunked allocation of the dummy backends' address space

Synopsis: The dummy backends allocate each bar's full address space up front,
which is slow for map files with huge, mostly unused address spaces. This change
allocates the address space lazily in fixed-size byte chunks, gives each
materialised SharedDummyBackend chunk its own shared-memory object, and reworks
the backdoor accessors accordingly.

Status: PLANNED

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
- A transfer helper maps `(bar, byteOffset, size)` onto the chunk map and is the
  only place that materialises chunks. It is used by:
   - `DummyBackend::read`/`write`;
   - `SharedDummyBackend::read`/`write`;
   - `writeRegisterWithoutCallback`;
   - the backdoor accessors.
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
- The home segment size is computed from these. The following are removed:
   - `getRequiredMemoryWithOverhead()`;
   - `getTotalRegisterSizeInBytes()`;
   - the now unused `SHARED_MEMORY_CONST_OVERHEAD`;
   - the now unused `SHARED_MEMORY_OVERHEAD_PER_VECTOR`.
- Names and versioning:
   - Base name: `Utilities::createShmName(instanceIdHash, mapFileName, user)`
     with the layout version appended,
     `<createdName>_v<SHARED_MEMORY_LAYOUT_VERSION>`.
   - `SHARED_MEMORY_LAYOUT_VERSION` is a `static constexpr`, bumped on every
     shared-memory layout change.
   - Everything derives from this base, so two library versions never attach to
     each other's objects, in either direction:
      - the home segment;
      - the interprocess mutex (base name, as today);
      - every chunk segment (`<base>_BAR_<bar>_CHUNK_<index>`).
   - The name is computed in one place, shared by the constructor and the
     stale-lock recovery path so the two cannot diverge.
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
      - A walker reaching a name whose segment does not yet exist (only while
        another process is between steps 2 and 3) treats it as the end of the
        chain and so misses nothing; prepending would hide the existing chain
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

### Backdoor accessors

- The backdoor accessors and their proxies store `(backend, bar, byteOffset)`
  instead of a raw pointer. The affected accessors are:
   - `DummyRegisterAccessor`;
   - `DummyMultiplexedRegisterAccessor`;
   - `DummyRegisterRawAccessor`.
- An element access copies exactly the element's bytes (its `RawType` size,
  which equals the element width) via the transfer helper (`memcpy`), keeping
  the `std::byte*` interface of `RawConverterCapsule`. The storage access is
  byte-granular, so neighbouring bytes are never modified.
- `DummyRegisterRawAccessor` becomes a class template over the raw type,
  `DummyRegisterRawAccessor<RawType = int32_t>`. A deduction guide keeps
  existing `DummyRegisterRawAccessor acc{...}` declarations compiling, and
  `DummyBackend::getRawAccessor` returns `DummyRegisterRawAccessor<>`.
- It returns value semantics instead of `int32_t&`; its proxy converts to and
  from `RawType` and implements, so that existing expressions (`raw += 5`,
  `raw++`, `raw & mask`, `~raw`) keep compiling and update the memory:
   - the compound-assignment operators (`+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`,
     `^=`, `<<=`, `>>=`);
   - pre/post-increment/decrement;
   - the binary operators `operator&` and `operator~`;
   - an implicit conversion to `RawType`.
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
  They materialise chunks under the `DummyBackend` mutex; all SharedDummyBackend
  materialisation goes through `read`/`write`/`writeRegisterWithoutCallback`
  under the interprocess mutex.

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
- Chunk boundaries: read/write of a register crossing a chunk boundary is
  correct; neighbouring chunks stay zero.
- Reading untouched addresses returns zeros; a write materialises only the
  touched chunk.
- Address access beyond any map-defined range works; reads yield zeros and
  writes materialise chunks on demand.
- SharedDummyBackend:
   - only used address ranges exist in shared memory; multi-process access still
     works (existing shared dummy tests);
   - a map file with an unreasonably huge address space (e.g. a 1000 TB
     register) opens and works while accessing only the first and the last few
     bytes; full reservation would exhaust any shared memory, so succeeding
     proves that nothing is reserved up front and that only the touched chunks
     materialise as shared objects;
   - a process using a map file with the same name but increased bar sizes joins
     an existing segment without crash or confusing errors;
   - the chunk chain is walked correctly, and after the last process leaves
     (also after a killed process is detected) no chunk segment of the instance
     is left in shared memory;
   - the pid-management and `shm_exists` based tests still pass:
      - `testSharedDummyBackendExt`;
      - `testSharedDummyBackendUnified` and its Ext variant;
      - `tests/scripts/testSharedDummyBackendPidManagement.sh`;
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
   - tests that assert sizes, full reservation or backend read-only state
     (`testDummyBackend`, `testDummyRegisterAccessor`) assert lazily
     materialised content only and no longer rely on the removed `setReadOnly`;
   - the out-of-range `logic_error` expectations follow the new "any address is
     valid" behaviour.
- Adapt the other backdoor and `AddressRange` consumers for the new accessor
  semantics:
   - `testGenericMuxedInterruptDistributor`;
   - `testNumericAddressedBackendRegisterAccessor`;
   - `testNumericAddressedBackendUnified`;
   - `testDoubleBufferAccessor`;
   - `testDummyBackendUnified`.
