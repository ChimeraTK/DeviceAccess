# CR-008: DAQ frame DMA ("ring buffer") support across backends

Synopsis: Add support for reading DAQ frames from a Xilinx AXI DMA S2MM
descriptor ring through virtual DMA channels. A register mapped to a virtual
DMA channel delivers the oldest unread DAQ frame; the feature is configured
entirely in the JMAP file and implemented once in a shared class used by the
XdmaBackend, the UioBackend and the Dummy backend. The Dummy backend offers a
backdoor to fill frames into the ring so the feature can be tested without
hardware.

Depends on: CR-007

Status: IN PROGRESS

## Requirements

Aspect: JMAP format.

- The `dmaChannels` section (introduced by CR-007) defines virtual DMA
  channels indexed by non-negative integers. Each entry carries the mandatory
  `type` key (validated by CR-007) and the backend-specific keys: the names
  of the DMA-engine control registers, the buffer allocator selection, the
  ring depth and the block size. The Xilinx ringbuffer uses the `type` value
  `"XilinxAxiS2MM"`.
- A register references a channel with the existing `address` object of type
  `"DMA"` whose `channel` is a channel *index* defined in `dmaChannels`; the
  register's `offset` is relative to the beginning of the DAQ frame.
- The frame size is constant in the first implementation, but the format must
  not make variable frame lengths impossible (e.g. a per-frame size or
  header-driven length is expressible).
- The hidden DMA-engine control registers are named in the `dmaChannels`
  section only; the register description never replicates them.

Aspect: virtual DMA channel as an addressable resource.

- A register that maps to a configured virtual DMA channel supports two read
  flavours, chosen through the standard API:
  - Push (accessor with `wait_for_new_data`): the channel delivers its oldest
    unread DAQ frame, one frame per delivered new-data event, oldest first.
    This is the production data path.
  - Poll (plain accessor read): the channel returns the newest fully written
    frame at the time the read transfer is initiated, on a best-effort basis.
    The read has no side effects: it neither advances the dequeue state, nor
    returns the served buffer to the engine, nor acknowledges the
    frame-arrival interrupt. For the duration of the read the served buffer is
    held out of the descriptor ring, so the engine cannot overwrite it and a
    torn frame can never be delivered; only whole frames are ever dropped
    (overrun reporting). Because polling never returns buffers to the engine,
    the ring is drained only by a running push consumer: a channel read only
    by poll freezes once the ring is full. Polling serves debugging tools
    (e.g. a register viewer) and never interferes with a concurrent push
    consumer (the push stream has no gaps).
- The generic `NumericAddressedBackend` needs no new structure: no new
  register type, no new accessor path. The frame channel is an ordinary
  addressable register.
- The feature is implemented once, in a shared reusable class: ring and
  dequeue state (which frame was last delivered), frame reassembly across ring
  blocks, ring advancement, buffer handling and overrun detection live in that
  class, and the XdmaBackend, the UioBackend and the Dummy backend all use the
  same implementation with almost no duplicated ring logic between them.
  Backend-specific hardware access is confined to a thin adapter per backend.
  Buffers are handed back to the engine only when consumed by a push read; the
  dequeue state advances only then, and a poll read never changes it.
- A consumer that lags the producer (ring overrun) is reported to the caller
  through a companion read-only register per channel declared in the JMAP.

Aspect: push mode.

- Frame channels support `wait_for_new_data` (the push read), driven by the
  existing `triggeredByInterrupt` mechanism. The push read delivers one frame
  per new-data notification, oldest first, and is the production data path.
  Only the push read returns consumed buffers to the engine, so the push
  stream is gap-free and the channel keeps producing new frames while a push
  consumer runs.

Aspect: slice coherence.

- Several registers may slice one frame at frame-relative offsets (e.g. a
  header register at offset 0 plus data registers after it). All slices of one
  frame share a single read so a consumer reading header and data gets a
  coherent snapshot of the same frame.

Aspect: ring ownership.

- The software owns the ring geometry by default: the ring is a fixed-size
  block ring in which a frame occupies a contiguous run of one or more blocks;
  for a known fixed frame size the block size is auto-derived so one block
  holds one frame, and ring depth (number of blocks) is the single tunable.
  An explicit block-size override covers firmware-fixed geometries. The
  firmware-expected variant is expressible via an ownership marker in the
  channel section.

Aspect: double buffering.

- Double buffering stays a separate, unchanged feature; it is not unified with
  the ring, and the ring is not a special case of it.

Aspect: backends.

- Implemented in a single shared, reusable class used by the `XdmaBackend`,
  the `UioBackend` and the Dummy backend, so all three backends support the
  same feature with almost no duplicated code.
- The Dummy backend offers a backdoor to fill DAQ frames into the ring, so the
  ring implementation and applications using it can be tested without
  hardware.

## Specifications

Affected components: `src/DmaRingBuffer.{h,cc}` (new shared ring-buffer class)
with a hardware-adapter interface, `backends/xdma/src/XdmaBackend.cc`,
`backends/uio/src/UioBackend.cc`, `backends/DummyBackend/src/DummyBackend.cc`,
`src/JsonMapFileParser.cc`, the `NumericAddressedRegisterCatalogue`,
`doc/jmapFormat.dox`, and the jmap test fixtures and parser tests.

Aspect: raw-json catch-all.

- The `dmaChannels` section is consumed through the generic backend-specific
  raw-json mechanism introduced by CR-007: entries are reached via
  `hasDmaChannel`/`getDmaChannel` on the register catalogue (held as the
  protected `_registerMap`). The parser preserves the section opaquely;
  each backend interprets it through the shared class; no
  `dmaChannels`-specific structure enters the generic
  `NumericAddressedRegisterCatalogue`.
- A frame-channel register's channel entry is interpreted only when its `type`
  value is the supported `"XilinxAxiS2MM"`; any other value raises
  `ChimeraTK::logic_error` (as required by CR-007).

Aspect: register-to-channel mapping.

- A register whose `address` has `type` `"DMA"` and a `channel` matching a
  channel index in `dmaChannels` is a frame-channel register. Its `offset` is
  the byte offset into the frame, not into a bar.
- Each frame-channel register is internally represented with two bar values,
  one per read flavour: a peek bar for ordinary reads and a pop bar for the
  interrupt-driven push read. Both bars address the same virtual DMA channel
  and frame offset; the bar values are reserved by the backend for its channel
  and are not bar numbers of the engine's register space.
- A single 2D multiplexed register at offset 0 represents the whole frame; its
  channel slices give structured access to the frame content. Several
  registers at frame-relative offsets give sub-region access. All slices of
  one frame share one read (one frame read serves all offset-slices),
  analogous to how a 2D register and its channel slices share one read today.

Aspect: shared ring-buffer class.

- A new reusable class holds all ring and frame logic and is used unchanged by
  the `XdmaBackend`, the `UioBackend` and the Dummy backend. It is driven
  through a small hardware-adapter interface, so the backends contain almost no
  duplicated ring logic; per backend only the adapter and the read entry point
  remain.
- The class owns the ring state, the dequeue cursor, the free/complete/served
  buffer sets and the per-channel overrun counter. `readFrame(peek|pop, frame
  offset, target)` performs one frame read for the whole frame (all
  offset-slices together), walks the descriptor run from `rxsof` to `rxeof` to
  reassemble the frame (which may span several ring blocks) and copies the
  concatenated bytes into the accessor buffer. A peek read returns the newest
  fully written frame without side effects; a pop read returns the oldest
  unread frame and advances the dequeue state.
- For the whole read the served frame's buffers are held out of the descriptor
  ring (they are not handed back to the engine), so the engine cannot overwrite
  them and a torn frame can never be delivered. Afterwards the pop read
  returns the consumed buffers to the engine; the peek read returns them to
  the set of complete frames (DONE), without touching the engine.
- A channel produces new frames only while a pop read returns buffers to the
  engine. A channel read only by poll freezes: the ring fills with complete
  frames, the engine stops (a backpressure-capable source pauses, a
  free-running source drops new data), and each peek read keeps returning the
  newest completed frame. This is intended and documented.
- The class performs the interrupt-driven handshaking of the push read
  (frame-arrival interrupt service/ack, S2MM status/control register
  reactions, consumed-buffer acknowledgement and overrun/overflow reporting
  when the consumer lags the producer) through the adapter; the actual
  interrupt wait and trigger stay in the backend. A poll read never
  acknowledges the frame-arrival interrupt.

Aspect: hardware adapters.

- The hardware-adapter interface abstracts what the shared class needs from a
  backend: buffer allocation/pinning and physical-address exposure (buffer
  allocator, e.g. `u-dma-buf` model), descriptor submission and completion
  status, S2MM controller start, and frame-arrival interrupt wait/ack. Each
  backend's `read()` selects the read flavour from the bar and delegates to the
  shared class.
- `XdmaBackend` implements the adapter on the existing `DmaIntf`/`CtrlIntf`/
  event infrastructure. The S2MM engine is configured for per-frame
  notification (IRQ threshold 1) and routed to an interrupt vector which the
  frame-channel register references (`triggeredByInterrupt`).
- `UioBackend` implements the adapter on its `UioAccess` (bar reads/writes to
  the S2MM registers) and its interrupt waiting (`activateSubscription`), so a
  real S2MM mapped into a UIO device is driven with the same class.
- The Dummy backend implements the adapter on an in-memory simulation of the
  engine and the buffer store, fed through the backdoor described below.

Aspect: overrun reporting.

- Each channel declares a companion read-only overrun register in the JMAP
  (a flag or counter). An overrun occurs when the engine needs a free buffer
  but none is available (the consumers cannot keep up, or no push consumer
  runs). Complete and served frames are never overwritten, so the only loss is
  of whole frames; the backend updates the register whenever such a loss is
  detected, so the application can poll it after a frame read.

Aspect: ring geometry derivation.

- The block is the minimum contiguous unit handed to the engine; a frame
  occupies a contiguous run of blocks (reassembly walks `rxsof`..`rxeof`). For
  a known fixed frame size the block size is auto-derived from the frame
  description (sum of the frame-relative regions, or the 2D register size), so
  one block holds one frame. A variable-length frame (per-header length, which
  the JMAP format keeps expressible) spans the corresponding number of blocks.
  Ring depth is a scalar in the channel section. An explicit block-size
  override applies when the channel declares firmware-owned geometry.

Aspect: Dummy backend backdoor.

- The Dummy backend offers a backdoor to fill DAQ frames into the ring, using
  the same convention as the existing `DUMMY_WRITEABLE` and
  `DUMMY_INTERRUPT_<N>` virtual registers.
- For each register of a DAQ frame, a virtual `DUMMY_WRITEABLE` register lets a
  test write that register's bytes; the written values are staged per channel.
  A virtual write-only trigger register per channel assembles the DAQ frame
  from the staged values, hands it to the shared ring-buffer class (the same
  implementation the real backends use) and triggers the frame-arrival
  interrupt, so `wait_for_new_data` fires and the regular push/poll read path
  serves the frame.
- This tests both the ring implementation itself and applications using a DAQ
  frame channel without hardware.

## Test plan

- Parser tests: a `dmaChannels` section with a register referencing a channel
  by index parses; register offsets are frame-relative.
- Dummy backend frame channel tests: a push read delivers the oldest unread
  frame one by one and advances the dequeue state; a poll read returns the
  newest frame and leaves the dequeue state, the engine and a concurrent push
  consumer untouched (no gaps in the push stream); with no push consumer the
  channel freezes once the ring is full (no new frames, the newest completed
  frame keeps being returned); several offset-slices of one frame return one
  coherent snapshot; `wait_for_new_data` fires on a new frame.
- Dummy backdoor tests: writing each frame register's `DUMMY_WRITEABLE` mirror
  and then the channel's trigger register fills a frame into the ring; a
  subsequent push read delivers it in order and `wait_for_new_data` fires; a
  poll read returns the newest filled frame. As all three backends use the
  same shared class, these tests exercise the ring implementation once for
  all of them.
- UioBackend: exercising its adapter requires the hardware UIO device; the
  shared ring logic is covered once through the Dummy backend.
- Overrun tests: a lagging consumer is reported through the companion
  register.
- Regression: the double-buffer tests keep passing (feature stays separate).
- Documentation: the `dmaChannels` section and the frame-channel register
  reference documented in `doc/jmapFormat.dox`.
