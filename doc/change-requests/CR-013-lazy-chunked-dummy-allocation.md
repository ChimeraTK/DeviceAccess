# CR-013: Lazy chunked allocation of the dummy backends' address space

Synopsis: The dummy backends allocate each bar's full address space up front,
which is slow for map files with huge, mostly unused address spaces. This
change allocates the address space lazily in fixed-size byte chunks and reworks
the backdoor accessors accordingly.

Status: PLANNED

## Requirements

- The address space of each bar is no longer fully allocated at construction
  (DummyBackend) resp. first open (SharedDummyBackend), but lazily in chunks.
- The chunk size is a `constexpr size_t` constant; no other code assumes a
  specific value.
- The `bar` and `address` parameters of `read`/`write` keep their current
  meaning; transfers are stitched transparently across chunk boundaries.
- Reading a never-allocated chunk yields zeros.
- Accessing an address outside the bar size given by the register mapping still
  throws `logic_error` as today.
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
- Bounds: a transfer fully beyond the bar size from
  `getBarSizesInBytesFromRegisterMapping()` throws `logic_error`, preserving the
  current `TRY_REGISTER_ACCESS` behaviour.
- SharedDummyBackend: chunk vectors are named `BAR_<bar>_CHUNK_<index>` and
  constructed via `findOrConstructVector`; `setupBarContents()` only resets the
  chunk maps; `reInitMemory()` still destroys all named vectors;
  `getRequiredMemoryWithOverhead()` reserves the worst case (all chunks) so the
  segment size limits are unchanged.
- `AddressRange`, `_readOnlyAddresses` and `_writeCallbackFunctions` remain
  address/range based and are unchanged.
- The backdoor accessors (`DummyRegisterAccessor`,
  `DummyMultiplexedRegisterAccessor`, `DummyRegisterRawAccessor`) and their
  proxies store `(backend, bar, byteOffset)` instead of a raw pointer; each
  element access copies the element bytes via the transfer helper (`memcpy`),
  keeping the `std::byte*` interface of `RawConverterCapsule`.
- `DummyRegisterRawAccessor` returns value semantics instead of `int32_t&`.
- The 32-bit alignment restrictions in the accessors (address and
  `elementPitchBits` multiples of 4) are removed.

### Alternatives considered

- Keep `int32_t` as internal storage type: rejected, keeps the reinterpret_casts
  and blocks byte-aligned accessors.
- Keep full per-bar allocation and only add chunking to SharedDummyBackend:
  rejected, the plain DummyBackend suffers the same construction-time cost.

## Test plan

- Construction resp. first open is fast for a map file with a large, mostly
  unused address space (no full-size zeroing).
- Chunk boundaries: read/write of a register crossing a chunk boundary is
  correct; neighbouring chunks stay zero.
- Reading untouched addresses returns zeros; a write materialises only the
  touched chunk.
- Out-of-range access still throws `logic_error`.
- SharedDummyBackend: only used address ranges exist in shared memory;
  multi-process access still works (existing shared dummy tests).
- Backdoor accessors: byte-aligned registers (odd byte offsets, misaligned
  pitch) work; registers spanning a chunk boundary work; existing accessor
  behaviour in testDummyRegisterAccessor is preserved; the raw accessor works
  with value semantics.
- Adapt the tests that access `_barContents` directly (testDummyBackend,
  testDummyRegisterAccessor, testSharedDummyBackend).
