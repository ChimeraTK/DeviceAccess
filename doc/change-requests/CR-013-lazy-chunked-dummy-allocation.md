# CR-013: Lazy chunked allocation of the dummy backends' address space

Synopsis: The dummy backends allocate each bar's full address space up front,
which is slow for map files with huge, mostly unused address spaces. This
change allocates the address space lazily in fixed-size byte chunks, materialises
each chunk of the SharedDummyBackend as its own shared-memory object, allows
map-less dummies, and reworks the backdoor accessors accordingly.

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
  file in a single shared segment; each chunk becomes its own shared-memory
  object, created on demand.
- Processes running different versions of the library must not silently share a
  shared segment; the shared segment records a layout version and rejects
  attaching a process whose version is incompatible.
- A map file with changed content under the same name must not crash or raise
  confusing errors, even when a bar size has grown.
- There is no restriction on the address range a bar may cover; any address is
  valid and materialises chunks on demand.
- Dummy backends (DummyBackend, SharedDummyBackend, ExceptionDummy) can be
  created without a map file; such a dummy has no registers but raw address
  access still works.
- The internal storage type of the bar contents changes from `int32_t` to
  `std::byte`.
- The backdoor accessors no longer expose references or pointers into the
  internal buffers; they return value-semantic proxies which handle the chunk
  discontinuities and support arbitrary byte-aligned registers (multi-byte
  access via `memcpy`).
- Read-only ranges and write-callback ranges keep their current per-address
  semantics.

## Specifications

- `_barContents` becomes a flat, chunked and byte-based map keyed by
  `(bar, chunkIndex)`; for the DummyBackend the value is
  `std::map<std::pair<uint64_t,uint64_t>, std::vector<std::byte>>` and for the
  SharedDummyBackend it is
  `std::map<std::pair<uint64_t,uint64_t>, SharedMemoryByteVector*>` with
  `SharedMemoryByteVector = boost::interprocess::vector<std::byte, ShmemByteAllocator>`.
- A new `constexpr size_t CHUNK_SIZE` (1 MiB) is defined in `DummyBackendBase`
  and used for all chunk-index arithmetic.
- Chunks are created zero-initialised on first access. Reads allocate missing
  chunks as well; only the touched chunk is materialised.
- A transfer helper maps `(bar, byteOffset, size)` onto the chunk map for reads
  and writes and is used by `DummyBackend::read`/`write`,
  `SharedDummyBackend::read`/`write` and `writeRegisterWithoutCallback`.
- No address range restriction: every `(bar, address)` access is valid and
  materialises chunks on demand; reads outside any map-defined range yield
  zeros.
- `getBarSizesInBytesFromRegisterMapping()` and the per-bar sizes derived from
  it become unused (removed with the sizing they served).
- SharedDummyBackend: the shared memory is organised as one small, stable home
  segment (holding the pid set, the interrupt semaphore array and the layout
  version) plus one separate shared-memory object per materialised chunk, named
  `BAR_<bar>_CHUNK_<index>`. Creating a chunk constructs and maps a new chunk
  object under the already-held interprocess mutex; it never relocates or
  remaps existing chunks, so cached `SharedMemoryByteVector*` pointers stay
  valid. A chunk object is created by the first process touching it and removed
  by the last process detaching (reusing the existing pid-set refcount pattern);
  `setupBarContents()` only resets the per-process chunk maps, and `reInitMemory()`
  destroys the materialised chunk objects together with the home segment when
  the last process leaves. If creating a chunk fails (e.g. `std::bad_alloc` or
  no space left in the shared memory filesystem), the operation throws
  `ChimeraTK::runtime_error` with a descriptive message. The `/dev/shm`
  file-count limit bounds the touched working set, not the map size.
  `getRequiredMemoryWithOverhead()` and `getTotalRegisterSizeInBytes()`
  (map-derived segment sizing) are removed.
- The map file does not limit the valid range of any bar; chunk vectors are
  always `CHUNK_SIZE` big, so a process joining a segment created by a process
  with a different map content can neither index out of range nor overrun;
  stale chunks of a formerly larger map simply stay unused.
- `createInstance` of DummyBackend, SharedDummyBackend and ExceptionDummy no
  longer rejects an empty `map` parameter; `NumericAddressedBackend` already
  handles an empty map file name (empty register catalogue).
- `_readOnlyAddresses` and `_writeCallbackFunctions` remain address/range
  based and keep their per-address semantics; `AddressRange::sizeInBytes`
  becomes 64-bit so a span of arbitrary size is never truncated.
- The backdoor accessors (`DummyRegisterAccessor`,
  `DummyMultiplexedRegisterAccessor`, `DummyRegisterRawAccessor`) and their
  proxies store `(backend, bar, byteOffset)` instead of a raw pointer; each
  element access copies the element bytes via the transfer helper (`memcpy`),
  keeping the `std::byte*` interface of `RawConverterCapsule`.
- `DummyRegisterRawAccessor` returns value semantics instead of `int32_t&`; its
  proxy implements the compound-assignment operators (`+=`, `-=`, `*=`, `/=`,
  `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=`) and
  pre/post-increment/decrement, so existing compound expressions and `++`/`--`
  keep compiling.
- The 32-bit alignment restrictions in the accessors (address and
  `elementPitchBits` multiples of 4) are removed.
- The byte offsets and sizes held by the backdoor accessors and their proxies
  (`_offsets`, `_nbytes`, `_pitch`, and `_nbytes` in `DummyRegisterElement`)
  are 64-bit, so the no-address-limit requirement holds for slabs above 4 GiB
  without truncation.
- All chunk materialisation and discovery, including when triggered by a
  backdoor accessor read or the interrupt dispatcher, happens under the
  interprocess mutex (shared) resp. the backend mutex (plain); accessor code
  never materialises or grows shared memory while holding only the per-process
  mutex.
- The home segment records a layout version and rejects attaching a process
  whose version is incompatible, so an old binary never silently joins a shared
  dummy built with the new chunk layout, and vice versa.

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
- Dummy backends can be created and opened without a map file; raw address
  access works (materialises chunks on demand), register access fails because
  the catalogue is empty.
- Backdoor accessors: byte-aligned registers (odd byte offsets, misaligned
  pitch) work; registers spanning a chunk boundary work; existing accessor
  behaviour in testDummyRegisterAccessor is preserved; the raw accessor works
  with value semantics and its compound-assignment/inc-dec operators (e.g.
  `raw += 5`, `raw++`) still compile and update the memory.
- Adapt the tests that assert sizes or full reservation of the bar contents
  (testDummyBackend, testDummyRegisterAccessor, testSharedDummyBackend) so they
  only assert lazily materialised content.
