# CR-013: Lazy chunked allocation of the dummy backends' address space

Synopsis: The dummy backends allocate each bar's full address space up front,
which is slow for map files with huge, mostly unused address spaces. This
change allocates the address space lazily in fixed-size byte chunks, materialises
each chunk of the SharedDummyBackend as its own shared-memory object, and
reworks the backdoor accessors accordingly.

Status: PLANNED

## Requirements

- The address space of each bar is no longer fully allocated at construction
  (DummyBackend) resp. first open (SharedDummyBackend), but lazily in chunks.
- The chunk size is a `constexpr size_t` constant; no other code assumes a
  specific value.
- The `bar` and `address` parameters of `read`/`write` keep their current
  meaning; transfers are stitched transparently across chunk boundaries.
- Reading a never-allocated chunk yields zeros.
- The SharedDummyBackend no longer reserves the full address space of the map
  file in a single shared segment; each materialised chunk becomes its own
  shared-memory object, created on demand, and the materialised chunks are
  tracked by a chain so they can be reset and removed without bounding their
  number.
- Processes running different versions of the library must never share a shared
  segment; the layout version is part of the shared-memory segment name, so a
  process built with an incompatible version derives a different name and
  cannot attach, in either direction.
- A map file with changed content under the same name must not crash or raise
  confusing errors, even when a bar size has grown.
- There is no restriction on the address range a bar may cover; any address is
  valid and materialises chunks on demand.
- The internal storage type of the bar contents changes from `int32_t` to
  `std::byte`.
- The backdoor accessors no longer expose references or pointers into the
  internal buffers; they return value-semantic proxies which handle the chunk
  discontinuities and support arbitrary byte-aligned registers (multi-byte
  access via `memcpy`).
- Read-only registers from the map file keep being rejected on write, but this
  enforcement stays entirely in the accessor layer.
- The interrupt storage of the SharedDummyBackend keeps its current bounded
  layout and is not chunked.

## Specifications

### Chunked bar storage

- `_barContents` becomes a flat, chunked and byte-based map keyed by
  `(bar, chunkIndex)`; for the DummyBackend the value is
  `std::map<std::pair<uint64_t,uint64_t>, std::vector<std::byte>>` and for the
  SharedDummyBackend it is
  `std::map<std::pair<uint64_t,uint64_t>, SharedMemoryByteVector*>` with
  `SharedMemoryByteVector = boost::interprocess::vector<std::byte, ShmemByteAllocator>`.
- The existing `ShmemAllocator` is also used for the `PidSet` and stays an
  `int32_t` allocator; a separate byte allocator type is introduced for the
  chunk vectors.
- A new `constexpr size_t CHUNK_SIZE` (1 MiB) is defined in `DummyBackendBase`
  and used for all chunk-index arithmetic.
- Chunks are created zero-initialised on first access. Reads allocate missing
  chunks as well; only the touched chunk is materialised.
- A transfer helper maps `(bar, byteOffset, size)` onto the chunk map for reads
  and writes. It is the single place that materialises chunks and is used by
  `DummyBackend::read`/`write`, `SharedDummyBackend::read`/`write`,
  `writeRegisterWithoutCallback` and the backdoor accessors.
- No address range restriction: every `(bar, address)` access is valid and
  materialises chunks on demand; reads outside any map-defined range yield
  zeros.
- `getBarSizesInBytesFromRegisterMapping()` and the per-bar sizes derived from
  it are removed, together with the sizing they served. The method is part of
  the installed `DummyBackendBase` API, so its removal is an accepted API break
  for downstream subclasses. It was also the only construction-time
  `elementPitchBits % 8` check at the DummyBackend level; the check in
  `NumericAddressedBackendRegisterAccessor` covers the accessor path.

### SharedDummyBackend shared memory

- The shared memory is organised as one small, fixed-size home segment plus one
  separate shared-memory object per materialised chunk. Chunk objects are named
  `<base>_BAR_<bar>_CHUNK_<index>`, so different map files, users and layout
  versions never collide, and objects are always removed by their full derived
  name.
- The home segment holds the pid set, the unchanged interrupt storage
  (`ShmForSems`: `semEntries[SHARED_MEMORY_N_MAX_MEMBER]` and
  `interruptEntries[maxInterruptEntries]`) and the head and tail of a singly
  linked list of the materialised chunk segments. Together they reach every
  materialised chunk; the list is grown at its tail, so the tail reference makes
  appending O(1) and is what lets a chunk be linked before it is created (see
  below). Its fixed size is computed from
  these, replacing `getRequiredMemoryWithOverhead()` and
  `getTotalRegisterSizeInBytes()`, which are removed together with the now
  unused `SHARED_MEMORY_CONST_OVERHEAD` and `SHARED_MEMORY_OVERHEAD_PER_VECTOR`.
- All object names derive from a common base name that carries the layout
  version. The base is the value of
  `Utilities::createShmName(instanceIdHash, mapFileName, user)` with the version
  appended, i.e. `<createdName>_v<SHARED_MEMORY_LAYOUT_VERSION>`, where
  `SHARED_MEMORY_LAYOUT_VERSION` is a `static constexpr` bumped whenever the
  shared-memory layout changes. The home segment, the interprocess mutex (which
  uses the same name, as today) and every chunk segment
  (`<base>_BAR_<bar>_CHUNK_<index>`) are derived from this base, so two library
  versions derive different names and can never attach to each other's objects,
  in either direction. The name is computed in one place, shared by the
  constructor and the stale-lock recovery path so the two cannot diverge. The
  previously unused `RequiredVersion` object in the home segment is removed.
- Each chunk segment carries a small fixed header with its `(bar, chunkIndex)`
  key and the name of the next segment in the chain; the last segment's
  successor is empty. The chain is in creation order, each new chunk being
  appended at the tail, and is not sorted by address or bar: a chunk's successor
  may belong to any bar or any address range. This is sufficient because the
  chain is only walked to reset or remove the chunks, never to look one up,
  which always goes by the derived name. The chain stores names, not mapped
  pointers, because mapped addresses differ between processes.
- A materialised `(bar, chunkIndex)` is appended at the tail in the same
  critical section of the already-held interprocess mutex, in three steps: the
  current tail's successor (or, for an empty chain, the head) is first set to
  the derived chunk name, then the segment is opened or created and its vector
  constructed, and only then is the home segment's tail advanced to the derived
  name. Writing the link before the segment exists means a created segment is
  always reachable from the head, and advancing the tail only after the segment
  exists means the tail never names a segment that does not exist, so recovery
  never has to walk the chain to re-derive the tail. Appending at the tail is
  what makes the early link safe: a walker that reaches a name whose segment does
  not yet exist - possible only while another process is midway through this
  sequence - treats it as the end of the chain, so it never misses a chunk,
  whereas prepending before creation would hide the whole existing chain behind
  the missing segment. A crash after the segment exists but before the tail is
  advanced leaves the tail at the previous segment; the new segment is already
  reachable from the head, so recovery removes it and never follows the stale
  tail. If creating the segment fails, the early link is rolled back under the
  mutex before the error is thrown.
- Chunks are never relocated or remapped once created, so the cached
  `SharedMemoryByteVector*` pointers in a process stay valid.
- There is no per-chunk refcount and no upper bound on the number of chunks; the
  chain is walked only when chunks must be reset or removed. The `/dev/shm`
  capacity bounds the touched working set, not the map size.
- On the crash-recovery path (`checkPidSetConsistency()` finds only dead pids),
  `reInitMemory()` walks the chain and removes or resets the chunk segments,
  instead of using `listNamedElements()`, which only ever saw the home segment.
  `setupBarContents()` only resets the per-process chunk maps.
- On teardown (the last process leaving), the head and tail are cleared under
  the mutex first, then the chunk segments are removed by walking the saved
  chain, and finally the home segment is removed. A process racing in sees an
  empty head and rebuilds, instead of following a chain whose segments are
  being removed.
- A crash inside the materialisation critical section cannot leak a segment:
  because the link is written before the segment is created, a created segment
  is always reachable and removed by teardown, while a linked name whose segment
  never came into existence is walked as the end of the chain.
- If creating a chunk fails (`std::bad_alloc` or a boost
  `interprocess_exception`, e.g. no space or no free inode in the shared-memory
  filesystem), the operation throws `ChimeraTK::runtime_error` with a
  descriptive message. This replaces the construction-time
  `ChimeraTK::logic_error` currently thrown by `setupBarContents()` on
  `bad_alloc`.

### Map file independence

- The map file does not limit the valid range of any bar; chunk vectors are
  always `CHUNK_SIZE` big, so a process joining a segment created by a process
  with a different map content can neither index out of range nor overrun;
  stale chunks of a formerly larger map simply stay unused.

### Callbacks and read-only

- `_writeCallbackFunctions` stays address/range based and keeps its per-address
  semantics; `AddressRange::sizeInBytes` becomes 64-bit so a span of arbitrary
  size is never truncated. The `AddressRange` layout change is an accepted ABI
  break; the header is installed.
- `DummyBackend::setReadOnly`, its `_readOnlyAddresses` set and `isReadOnly()`
  are removed; `isWriteRangeOverlap()` then collapses to a pure overlap test and
  is simplified accordingly.
- Read-only registers from the map file are enforced at the accessor layer
  (`NumericAddressedBackendRegisterAccessor` throws `logic_error` on a write to
  a non-writeable register), so a read-only write through the normal Device
  interface never reaches the backend and the backend-level skip is dead in
  practice. `writeRegisterWithoutCallback` continues to bypass any read-only
  protection, as it must for resync inside write callbacks, and therefore
  performs no read-only filtering.
- Two consequences are accepted: a direct `DummyBackend::write` (a public method
  used by tests) to a read-only mapped address now succeeds, and reads or writes
  beyond the map-defined bar size no longer throw `logic_error` (any address is
  valid). The `out_of_range` handling in the `TRY_REGISTER_ACCESS` macro becomes
  unused.

### Backdoor accessors

- The backdoor accessors (`DummyRegisterAccessor`,
  `DummyMultiplexedRegisterAccessor`, `DummyRegisterRawAccessor`) and their
  proxies store `(backend, bar, byteOffset)` instead of a raw pointer; each
  element access copies the element bytes via the transfer helper (`memcpy`),
  keeping the `std::byte*` interface of `RawConverterCapsule`.
- The proxies carry the backend, bar and byte offset needed for that copy, and
  an element access performs a read-modify-write of the whole element so that
  neighbouring bytes in a partial word are preserved.
- `DummyRegisterRawAccessor` returns value semantics instead of `int32_t&`; its
  proxy implements the compound-assignment operators (`+=`, `-=`, `*=`, `/=`,
  `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=`), pre/post-increment/decrement, the
  binary `operator&`, `operator~` and an implicit conversion to the underlying
  type, so existing expressions (`raw += 5`, `raw++`, `raw & mask`, `~raw`)
  keep compiling and updating the memory.
- The 32-bit alignment restrictions in the accessors are removed: the
  `address % sizeof(int32_t)` and `elementPitchBits % (8 * sizeof(int32_t))`
  checks in `DummyRegisterAccessor::getElement`, and the dependent word-strided
  indexing in `DummyMultiplexedRegisterAccessor::operator[]` and
  `proxies::DummyRegisterSequence`, are replaced by byte offsets. The
  `elementPitchBits % 8 == 0` constraint remains, as element pitches below one
  byte are not supported.
- The byte offsets and sizes held by the backdoor accessors and their proxies
  are 64-bit throughout, including `_offsets`, `_nbytes`, `_pitch` and
  `DummyRegisterElement::_nbytes`, and `DummyRegisterAccessor::setWriteCallback`
  no longer truncates `bar` to `uint8_t` or `address` to `uint32_t`, so slabs
  above 4 GiB are not truncated.
- The backdoor accessors exist only on the plain `DummyBackend` and take a
  `DummyBackend&`; `SharedDummyBackend` derives from `DummyBackendBase` and has
  none. They therefore materialise chunks under the `DummyBackend` mutex, while
  all SharedDummyBackend materialisation goes through
  `read`/`write`/`writeRegisterWithoutCallback` under the interprocess mutex.

### Alternatives considered

- Keep `int32_t` as internal storage type: rejected, keeps the reinterpret_casts
  and blocks byte-aligned accessors.
- Keep full per-bar allocation and only add chunking to SharedDummyBackend:
  rejected, although the plain DummyBackend does not really suffer from the
  construction cost, chunking both keeps them in sync and avoids supporting two
  storage models in the shared transfer helper and the backdoor accessors.
- Reserve the full map address space in the shared segment or key the segment
  name by a map content hash: rejected, the shared memory filesystem (e.g. 64
  MiB `/dev/shm` in containers) is often too small for a full reservation and
  creating the segment would be slow.
- Grow a single shared segment on demand via `managed_shared_memory::grow`:
  rejected, Boost.Interprocess only offers off-line growing (every process must
  unmap first) and remaps the segment, invalidating the raw pointers which
  address the shared chunks.
- Track the materialised chunks in a fixed registry in the home segment:
  rejected, a `managed_shared_memory` segment cannot grow, so the registry would
  impose an artificial reserved upper bound; the linked chain is unbounded and
  is only walked on reset and teardown.
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
- SharedDummyBackend: only used address ranges exist in shared memory;
  multi-process access still works (existing shared dummy tests).
- SharedDummyBackend: a map file with an unreasonably huge address space (e.g.
  a 1000 TB register) opens and works while accessing only the first and the
  last few bytes; full reservation would exhaust any shared memory, so
  succeeding proves nothing is reserved up front and only the touched chunks
  materialise as shared objects.
- SharedDummyBackend: a process using a map file with the same name but
  increased bar sizes joins an existing segment without crash or confusing
  errors.
- SharedDummyBackend: the chunk chain is walked correctly, and after the last
  process leaves (also after a killed process is detected) no chunk segment of
  the instance is left in shared memory; the pid-management and `shm_exists`
  based tests (`testSharedDummyBackendExt`, `testSharedDummyBackendUnified` and
  its Ext variant, `tests/scripts/testSharedDummyBackendPidManagement.sh`) still
  pass.
- SharedDummyBackend: processes built with different layout versions use
  different segment names and never attach to each other's segment.
- Backdoor accessors: byte-aligned registers (odd byte offsets, misaligned
  pitch) work; registers spanning a chunk boundary work; existing accessor
  behaviour in testDummyRegisterAccessor is preserved; the raw accessor works
  with value semantics and its compound-assignment/inc-dec, binary `&`, `~`, and
  conversion operators (e.g. `raw += 5`, `raw++`, `raw & mask`) still compile
  and update the memory.
- Adapt the tests that assert sizes, full reservation or backend read-only state
  (testDummyBackend, testDummyRegisterAccessor) so they only assert lazily
  materialised content and no longer rely on the removed `setReadOnly`, and
  adapt the out-of-range `logic_error` expectations to the new "any address is
  valid" behaviour. The other backdoor and `AddressRange` consumers
  (`testGenericMuxedInterruptDistributor`,
  `testNumericAddressedBackendRegisterAccessor`,
  `testNumericAddressedBackendUnified`, `testDoubleBufferAccessor`,
  `testDummyBackendUnified`) are updated for the new accessor semantics.
