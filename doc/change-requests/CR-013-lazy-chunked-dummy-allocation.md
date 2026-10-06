# CR-013: Lazy chunked allocation of the dummy backends' address space

Synopsis: The dummy backends allocate each bar's full address space up front,
which is slow for map files with huge, mostly unused address spaces. This
change allocates the address space lazily in fixed-size byte chunks, grows the
shared-memory segment of the SharedDummyBackend dynamically, allows map-less
dummies, and reworks the backdoor accessors accordingly.

Status: PLANNED

## Requirements

- The address space of each bar is no longer fully allocated at construction
  (DummyBackend) resp. first open (SharedDummyBackend), but lazily in chunks.
- The chunk size is a `constexpr size_t` constant; no other code assumes a
  specific value.
- The `bar` and `address` parameters of `read`/`write` keep their current
  meaning; transfers are stitched transparently across chunk boundaries.
- Reading a never-allocated chunk yields zeros.
- The SharedDummyBackend shared segment no longer reserves the full address
  space of the map file; it starts small and grows dynamically with the chunks.
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

- `_barContents` becomes chunked and byte-based:
  - DummyBackend: `std::map<uint64_t, std::map<uint64_t, std::vector<std::byte>>>`
    (bar -> chunk index -> chunk data).
  - SharedDummyBackend: `std::map<uint64_t, std::map<uint64_t, SharedMemoryByteVector*>>`
    with `SharedMemoryByteVector = boost::interprocess::vector<std::byte, ShmemByteAllocator>`.
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
- SharedDummyBackend: chunk vectors are named `BAR_<bar>_CHUNK_<index>` and
  constructed via `findOrConstructVector`; `setupBarContents()` only resets the
  chunk maps; `reInitMemory()` still destroys all named vectors.
- SharedDummyBackend: the segment is created with a small fixed initial size
  independent of the map file; if a chunk allocation does not fit the free
  memory, the segment is grown via `managed_shared_memory::grow` by a multiple
  of `CHUNK_SIZE` under the already-held interprocess mutex, then the
  allocation is retried. `getRequiredMemoryWithOverhead()` and
  `getTotalRegisterSizeInBytes()` (map-derived segment sizing) are removed.
- The map file does not limit the valid range of any bar; chunk vectors are
  always `CHUNK_SIZE` big, so a process joining a segment created by a process
  with a different map content can neither index out of range nor overrun;
  stale chunks of a formerly larger map simply stay unused.
- `createInstance` of DummyBackend, SharedDummyBackend and ExceptionDummy no
  longer rejects an empty `map` parameter; `NumericAddressedBackend` already
  handles an empty map file name (empty register catalogue).
- `AddressRange`, `_readOnlyAddresses` and `_writeCallbackFunctions` remain
  address/range based and are unchanged.
- The backdoor accessors (`DummyRegisterAccessor`,
  `DummyMultiplexedRegisterAccessor`, `DummyRegisterRawAccessor`) and their
  proxies store `(backend, bar, byteOffset)` instead of a raw pointer; each
  element access copies the element bytes via the transfer helper (`memcpy`),
  keeping the `std::byte*` interface of `RawConverterCapsule`.
- `DummyRegisterRawAccessor` returns value semantics instead of `int32_t&`; its
  proxy implements the compound-assignment operators (`+=`, `-=`, `*=`, `/=`,
  `&=`, `|=`, `^=`, `<<=`, `>>=`) and pre/post-increment/decrement, so existing
  compound expressions and `++`/`--` keep compiling.
- The 32-bit alignment restrictions in the accessors (address and
  `elementPitchBits` multiples of 4) are removed.

### Alternatives considered

- Keep `int32_t` as internal storage type: rejected, keeps the reinterpret_casts
  and blocks byte-aligned accessors.
- Keep full per-bar allocation and only add chunking to SharedDummyBackend:
  rejected, the plain DummyBackend suffers the same construction-time cost.
- Reserve the full map address space in the shared segment or key the segment
  name by a map content hash: rejected, the shared memory filesystem (e.g. 64
  MiB `/dev/shm` in containers) is often too small for a full reservation and
  creating the segment would be slow; dynamic growth under the existing
  interprocess mutex avoids both.

## Test plan

- Construction resp. first open is fast for a map file with a large, mostly
  unused address space (no full-size zeroing).
- Chunk boundaries: read/write of a register crossing a chunk boundary is
  correct; neighbouring chunks stay zero.
- Reading untouched addresses returns zeros; a write materialises only the
  touched chunk.
- Address access beyond any map-defined range works; reads yield zeros and
  writes materialise chunks on demand.
- SharedDummyBackend: only used address ranges exist in shared memory;
  multi-process access still works (existing shared dummy tests).
- SharedDummyBackend: a map file whose address space exceeds the available
  shared memory still opens and works, because nothing is reserved up front;
  the segment grows on demand.
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
- Adapt the tests that access `_barContents` directly (testDummyBackend,
  testDummyRegisterAccessor, testSharedDummyBackend).
