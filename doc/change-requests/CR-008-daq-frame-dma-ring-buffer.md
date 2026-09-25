# CR-008: DAQ frame DMA ("ring buffer") support in the XdmaBackend

Synopsis: Add support for reading DAQ frames from a Xilinx AXI DMA S2MM
descriptor ring through virtual DMA channels. A register mapped to a virtual
DMA channel delivers the oldest unread DAQ frame; the feature is configured
entirely in the JMAP file, implemented in the XdmaBackend and mirrored in the
Dummy backend for testing.

Depends on: CR-007

Status: IN PROGRESS

## Requirements

Aspect: virtual DMA channel as an addressable resource.

- A register that maps to a configured virtual DMA channel supports two read
  flavours, chosen through the standard API:
  - Push (accessor with `wait_for_new_data`): the channel delivers its oldest
    unread DAQ frame, one frame per delivered new-data event, oldest first.
    This is the production data path.
  - Poll (plain accessor read): the channel returns the newest fully written
    frame at the time the read transfer is initiated, on a best-effort basis,
    with no side effects (no dequeue advance, no buffer release, no interrupt
    acknowledgement). The data may be corrupted if the ring completes a full
    turn and overwrites the buffer during the read. This serves debugging
    tools (e.g. a register viewer) and must not interfere with a concurrent
    push consumer.
- The generic `NumericAddressedBackend` needs no new structure: no new
  register type, no new accessor path. The frame channel is an ordinary
  addressable register.
- All DMA-engine work and all handshaking happen inside `XdmaBackend::read()`:
  buffer allocation and pinning, physical-address exposure, descriptor-ring
  programming, S2MM controller start, dequeue state (which frame was last
  delivered), frame reassembly across ring blocks, ring advancement, interrupt
  servicing and acknowledgement, and overrun detection. The dequeue state
  advances only on a push read; a poll read never changes it.
- A consumer that lags the producer (ring overrun) is reported to the caller
  through a companion read-only register per channel declared in the JMAP.

Aspect: push mode.

- Frame channels support `wait_for_new_data` (the push read), driven by the
  existing `triggeredByInterrupt` mechanism. The push read delivers one frame
  per new-data notification, oldest first, and is the production data path.

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

Aspect: slice coherence.

- Several registers may slice one frame at frame-relative offsets (e.g. a
  header register at offset 0 plus data registers after it). All slices of one
  frame share a single read so a consumer reading header and data gets a
  coherent snapshot of the same frame.

Aspect: ring ownership.

- The software owns the ring geometry by default: block size is auto-derived
  to equal one frame, ring depth (number of blocks) is the single tunable.
  An explicit block-size override covers firmware-fixed geometries. The
  firmware-expected variant is expressible via an ownership marker in the
  channel section.

Aspect: double buffering.

- Double buffering stays a separate, unchanged feature; it is not unified with
  the ring, and the ring is not a special case of it.

Aspect: backends.

- Implemented in the `XdmaBackend`, mirrored in the Dummy backend so the
  feature can be tested without hardware.

## Specifications

Affected components: `backends/xdma/src/XdmaBackend.cc`,
`backends/dummy/...`, `src/JsonMapFileParser.cc`, the
`NumericAddressedRegisterCatalogue`, `doc/jmapFormat.dox`, and the jmap test
fixtures and parser tests.

Aspect: raw-json catch-all.

- The `dmaChannels` section is consumed through the generic backend-specific
  raw-json mechanism introduced by CR-007: entries are reached via
  `hasDmaChannel`/`getDmaChannel` on the register catalogue (held as the
  protected `_registerMap`). The parser preserves the section opaquely;
  `XdmaBackend` interprets it. No `dmaChannels`-specific structure enters the
  generic `NumericAddressedRegisterCatalogue`.
- `XdmaBackend` interprets a frame-channel register's channel entry only when
  its `type` value is the supported `"XilinxAxiS2MM"`; any other value raises
  `ChimeraTK::logic_error` (as required by CR-007).

Aspect: register-to-channel mapping.

- A register whose `address` has `type` `"DMA"` and a `channel` matching a
  channel index in `dmaChannels` is a frame-channel register. Its `offset` is
  the byte offset into the frame, not into a bar.
- Each frame-channel register is internally represented with two bar values,
  one per read flavour: a peek bar for ordinary reads and a pop bar for the
  interrupt-driven push read. Both bars address the same virtual DMA channel
  and frame offset; the bar values are reserved by the `XdmaBackend` and are
  not bar numbers of the engine's register space.
- A single 2D multiplexed register at offset 0 represents the whole frame; its
  channel slices give structured access to the frame content. Several
  registers at frame-relative offsets give sub-region access. All slices of
  one frame share one read (one frame read serves all offset-slices),
  analogous to how a 2D register and its channel slices share one read today.

Aspect: `XdmaBackend::read()`.

- `read()` selects the read flavour from the bar: a peek-bar read returns the
  newest fully written frame without side effects; a pop-bar read returns the
  oldest unread frame and advances the dequeue state. Both perform one frame
  read for the whole frame (all offset-slices together).
- Frame read: allocate/pin host buffers and expose their physical addresses
  (buffer allocator, e.g. `u-dma-buf` model), program the S2MM descriptor
  ring, start the S2MM controller, walk the descriptors from `rxsof` to
  `rxeof` to reassemble the frame (which may span several ring blocks), and
  copy the concatenated bytes into the accessor buffer. The pop read
  additionally clears `cmplt` to acknowledge, releases the consumed buffers
  and advances the dequeue state.
- All handshaking stays inside the read path: frame-arrival interrupt
  service/ack, S2MM status/control register reactions, consumed-buffer
  acknowledgement, and overrun/overflow reporting when the consumer lags the
  producer. The interrupt-driven push read acknowledges the frame-arrival
  interrupt; a poll read never does.
- The push read is triggered by the S2MM engine's frame-arrival interrupt,
  configured for per-frame notification (S2MM IRQ threshold 1) and routed to
  an interrupt vector which the frame channel register references
  (`triggeredByInterrupt`).
- Builds on the existing `DmaIntf`/`CtrlIntf`/event infrastructure.

Aspect: overrun reporting.

- Each channel declares a companion read-only overrun register in the JMAP
  (a flag or counter). The backend updates it when a ring overrun is detected,
  so the application can poll it after a frame read.

Aspect: ring geometry derivation.

- Block size auto-derived from the frame description (sum of the
  frame-relative regions, or the 2D register size), so one block holds one
  frame and a frame never spans blocks. Ring depth is a scalar in the channel
  section. An explicit block-size override applies when the channel declares
  firmware-owned geometry.

Aspect: Dummy backend mirror.

- The Dummy backend gains a frame-channel variant exposing fresh frames so the
  read/reassembly/overrun path is testable without hardware.

## Test plan

- Parser tests: a `dmaChannels` section with a register referencing a channel
  by index parses; register offsets are frame-relative.
- Dummy backend frame channel tests: a push read delivers the oldest unread
  frame one by one and advances the dequeue state; a poll read returns the
  newest frame and leaves the dequeue state and a concurrent push consumer
  untouched; several offset-slices of one frame return one coherent snapshot;
  `wait_for_new_data` fires on a new frame.
- Overrun tests: a lagging consumer is reported through the companion
  register.
- Regression: the double-buffer tests keep passing (feature stays separate).
- Documentation: the `dmaChannels` section and the frame-channel register
  reference documented in `doc/jmapFormat.dox`.
