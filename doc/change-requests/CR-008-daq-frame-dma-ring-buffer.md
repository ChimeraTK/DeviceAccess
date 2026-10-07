# CR-008: DAQ frame DMA ("ring buffer") support across backends

Synopsis: Read DAQ frames from a Xilinx AXI DMA S2MM descriptor ring through
virtual DMA channels. The feature is configured in the JMAP file and
implemented once in a shared class used by the XdmaBackend, the UioBackend and
the Dummy backend. The Dummy backend offers a backdoor to fill frames, so the
feature can be tested without hardware.

Depends on: CR-007

Status: IN PROGRESS

## Requirements

- The `dmaChannels` section (CR-007) defines virtual DMA channels keyed by
  non-negative integer indices. The Xilinx ring buffer uses the `type` value
  `"XilinxAxiS2MM"`.
- An `"XilinxAxiS2MM"` entry has these keys:
- `controlRegister` (string): path of the S2MM DMACR register.
- `statusRegister` (string): path of the S2MM DMASR register.
- `currentDescriptorRegister` (string): path of the S2MM CURDESC register.
- `tailDescriptorRegister` (string): path of the S2MM TAILDESC register.
- `ringDepth` (integer): number of blocks in the descriptor ring.
- `descriptorBuffer` (object): where the descriptor ring lives. It carries the
  udmabuf `device` path. The ring is always host-pinned, because the S2MM engine
  reads it over `M_AXI_SG`.
- `dataBuffer` (object): where the S2MM target buffers live.
- `dataBuffer.location` (string): `"host"` or `"fpgaDdr"`.
- `dataBuffer.device` (string): udmabuf `device` path, required for `"host"`.
- `dataBuffer.dmaChannel` (integer, default 0): classic DMA channel used to
  read the FPGA memory, used for `"fpgaDdr"`.
- `dataBuffer.baseAddress` (integer): byte address of the buffer area in the
  FPGA memory, used for `"fpgaDdr"`.
- `dataBuffer.size` (integer): buffer-area size in bytes, used for `"fpgaDdr"`.
- `overrunRegister` (string): path of the read-only overrun register.
- `overrunOffset` (integer): byte offset of the overrun register in the virtual
  bar.
- `blockSize` (integer, optional): block-size override in bytes. The default is
  derived from the frame size.
- `ownership` (string, optional): `"software"` (default) or `"firmware"`.
- The DMA-engine registers are hidden. They are named in the `dmaChannels`
  entry only. The frame-channel register description does not repeat them.
- A frame-channel register uses the existing `address` object with `type`
  `"DMA"`. Its `channel` is a `dmaChannels` index. Its `offset` is relative to
  the start of the DAQ frame.
- A frame-channel register names the S2MM frame-arrival interrupt through the
  existing `triggeredByInterrupt` field.
- The frame size is constant in the first implementation. The format must keep
  variable frame lengths expressible.
- A frame-channel register supports two read flavours, selected by the
  standard API:
- push: an accessor with `wait_for_new_data`. It delivers the oldest unread
  frame, one frame per new-data event, oldest first. This is the production data
  path.
- poll: a plain accessor read. It returns the newest complete frame at the
  time of the read, best effort.
- A poll read has no side effects:
- it does not advance the dequeue state;
- it does not return served buffers to the engine;
- it does not acknowledge the interrupt.
- A poll read holds the served buffer out of the ring for the read's duration,
  so a torn frame is never delivered. Only whole frames can be lost.
- Only a push read returns consumed buffers to the engine. A channel read only
  by poll freezes once the ring is full.
- A poll read never disturbs a concurrent push consumer.
- Several registers may slice one frame at frame-relative offsets, for example
  a header at offset 0 and data after it.
- All slices of one frame share a single read. A consumer reading header and
  data therefore gets one coherent snapshot of the same frame.
- The software owns the ring geometry by default. The ring is a fixed-size
  block ring in which a frame occupies a contiguous run of one or more blocks.
- For a known fixed frame size the block size is auto-derived so one block
  holds one frame. The ring depth is the single tunable.
- An explicit `blockSize` covers firmware-fixed geometries.
- `ownership` expresses a firmware-expected variant.
- Double buffering stays a separate, unchanged feature. The ring is not a
  special case of it.
- One shared reusable class implements the ring logic. The `XdmaBackend`, the
  `UioBackend` and the Dummy backend all use it.
- The Dummy backend offers a backdoor to fill frames into the ring, so the
  feature and its applications can be tested without hardware.
- Each channel declares a companion read-only overrun register in the JMAP. It
  is a flag or a counter.
- An overrun occurs when the engine needs a free buffer but none is available.
  Complete and served frames are never overwritten, so only whole frames can be
  lost.
- The backend updates the overrun register whenever such a loss is detected.
  The application can poll it after a frame read.
- The overrun register is a virtual register in a backend-reserved virtual bar
  (bar 12). It is read-only. A write raises `ChimeraTK::logic_error`. It does
  not support `wait_for_new_data`.

## Specifications

Affected components: the new shared class `src/DmaRingBuffer.{h,cc}` with a
hardware-adapter interface, `backends/xdma/src/XdmaBackend.cc`,
`backends/uio/src/UioBackend.cc`, `backends/DummyBackend/src/DummyBackend.cc`,
`src/JsonMapFileParser.cc`, the `NumericAddressedRegisterCatalogue`,
`doc/jmapFormat.dox`, jmap fixtures and parser tests.

### Raw-json DMA channel configuration

- The `dmaChannels` section is consumed through the CR-007 raw-json
  configuration. Entries are reached through `hasDmaChannel` and `getDmaChannel`
  on the register catalogue.
- The parser preserves the section opaquely. Each backend interprets it
  through the shared class. No `dmaChannels` structure enters the generic
  `NumericAddressedRegisterCatalogue`.
- A channel entry is interpreted only when its `type` is `"XilinxAxiS2MM"`.
  Any other value raises `ChimeraTK::logic_error`.

### Register-to-channel mapping

- A register whose `address` has `type` `"DMA"` and a `channel` matching a
  `dmaChannels` index is a frame-channel register. Its `offset` is a byte offset
  into the frame, not into a bar.
- Each frame-channel register has two internal bar values, one per read
  flavour: a peek bar for plain reads and a pop bar for the interrupt-driven
  push read. Both address the same virtual channel and frame offset. The bar
  values are backend-reserved and are not engine register-space bars.
- The overrun register is an ordinary catalogue register at the virtual
  address `overrunOffset` in virtual bar 12. The backend overrides
  `barIndexValid` to admit bar 12 and serves its reads from the shared class.
- The virtual bar must not collide with the per-channel peek and pop bars.
- A single 2D multiplexed register at offset 0 represents the whole frame. Its
  channel slices give structured access. Several registers at frame-relative
  offsets give sub-region access. All slices share one read.

### Shared ring-buffer class

- The class holds all ring and frame logic. The three backends use it
  unchanged. It is driven through a small hardware-adapter interface, so the
  backends contain almost no duplicated ring logic.
- It owns the ring state and the dequeue cursor.
- It owns the buffer sets:
- free;
- complete;
- served.
- It owns the per-channel overrun counter.
- `readFrame(peek|pop, frameOffset, target)` performs one read for the whole
  frame and copies the concatenated bytes into the accessor buffer.
- The read walks the descriptor run from `rxsof` to `rxeof`, so a frame may
  span several blocks.
- A peek read returns the newest complete frame without side effects. A pop
  read returns the oldest unread frame and advances the dequeue state.
- During the read the frame's buffers are held out of the ring, so a torn frame
  is never delivered. Afterwards a pop read returns them to the engine. A peek
  read returns them to the complete set without touching the engine.
- A channel produces new frames only while a pop read returns buffers to the
  engine. A poll-only channel freezes once the ring is full.
- The class performs the push handshake through the adapter:
- interrupt service and acknowledgement;
- S2MM status and control register reactions;
- handing consumed buffers back;
- the overrun count.
- The interrupt wait and trigger stay in the backend.
- A poll read never acknowledges the interrupt.
- Ring geometry: the block is the minimum contiguous unit handed to the engine.
  A frame occupies a contiguous run of blocks. For a known fixed frame size the
  block size is auto-derived so one block holds one frame. `blockSize` overrides
  it. The number of blocks is `ringDepth` from the channel entry.

### Hardware adapters

- The adapter abstracts what the shared class needs:
- buffer allocation and pinning, with physical-address exposure;
- descriptor submission and completion status;
- S2MM controller start;
- interrupt wait and acknowledgement.
- The adapter exposes a buffer-read primitive for both `dataBuffer.location`
  values: a mapped zero-copy read for `"host"` and a classic-DMA copy (`pread`
  on `c2hN`) for `"fpgaDdr"`.
- The descriptor ring is host-pinned in both cases. The S2MM target address is
  the descriptor buffer address, so `dataBuffer.location` selects the physical
  memory the engine writes into.
- Each backend's `read()` selects the read flavour from the bar and delegates
  to the shared class.
- `XdmaBackend`: the adapter uses the existing `DmaIntf`, `CtrlIntf` and event
  infrastructure. The S2MM engine is configured for per-frame notification (IRQ
  threshold 1) and routed to an interrupt vector referenced by the frame-channel
  register.
- `UioBackend`: the adapter uses `UioAccess` for the S2MM registers and
  `activateSubscription` for the interrupt wait, so a real S2MM mapped into a
  UIO device is driven with the same class.
- Dummy backend: the adapter uses an in-memory simulation of the engine and the
  buffer store, fed through the backdoor.

### Dummy backend backdoor

- The backdoor follows the convention of the existing `DUMMY_WRITEABLE` and
  `DUMMY_INTERRUPT_<N>` virtual registers.
- For each frame register, a virtual `DUMMY_WRITEABLE` mirror lets a test write
  that register's bytes. The values are staged per channel.
- A virtual write-only trigger register per channel assembles a frame from the
  staged values, hands it to the shared class and triggers the frame-arrival
  interrupt.
- The regular push and poll read path then serves the frame, so
  `wait_for_new_data` fires.

### Alternatives considered

- S2MM metadata (APP0 to APP4) support: deferred. The engine stores the AXI
  Status Stream only in the EOF descriptor, so the granularity is per frame, not
  per buffer. A real use case is needed to fix the layout and the access model
  before a format can be defined.

## Test plan

- Parser tests: a `dmaChannels` section with a register referencing a channel
  by index parses. Register offsets are frame-relative.
- Dummy backend frame channel:
- a push read delivers the oldest unread frame one by one and advances the
  dequeue state;
- a poll read returns the newest frame and leaves the dequeue state, the engine
  and a concurrent push consumer untouched;
- with no push consumer the channel freezes once the ring is full and the
  newest complete frame keeps being returned;
- several offset-slices of one frame return one coherent snapshot;
- `wait_for_new_data` fires on a new frame.
- Dummy backdoor: writing each frame register's `DUMMY_WRITEABLE` mirror and
  then the channel trigger fills a frame. A push read delivers it in order. A
  poll read returns the newest filled frame. As all three backends share the
  class, this exercises the ring implementation once for all of them.
- Overrun: a lagging consumer is reported through the companion register.
- `fpgaDdr`: the classic-DMA buffer copy is exercised through the Dummy
  backend. On hardware it needs the FPGA memory area.
- UioBackend: its adapter needs the hardware UIO device. The shared ring logic
  is covered through the Dummy backend.
- Regression: the double-buffer tests keep passing.
- Documentation: the `dmaChannels` section and the frame-channel register
  reference are documented in `doc/jmapFormat.dox`.
