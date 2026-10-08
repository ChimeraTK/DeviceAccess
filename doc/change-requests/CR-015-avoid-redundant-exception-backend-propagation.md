# CR-015: Avoid redundant exception backend propagation in replaceTransferElement

Synopsis: `replaceTransferElement()` propagates the exception backend to its
target after the replacement step even when nothing was replaced. Where the call
is forwarded to the target, that target already propagates, so the second
propagation is redundant.

Status: PLANNED

## Requirements

- A `replaceTransferElement()` implementation must not propagate the exception
  backend to its target when it only forwards the call to that target.
- After any replacement the resulting accessor chain carries this decorator's
  exception backend, as before.
- No change to which backend a chain ends up with; only redundant calls are
  removed.
- `TransferGroup::addAccessorImpl()` continues to produce the same wiring, with
  fewer `setExceptionBackend()` invocations.

## Specifications

Affected components (the `replaceTransferElement()` implementations of):

- `NDRegisterAccessorDecorator` (`include/NDRegisterAccessorDecorator.h`).
- `BitRangeAccessorDecorator` (`include/BitRangeAccessorDecorator.h`).
- `SubArrayAccessorDecorator` (`include/SubArrayAccessorDecorator.h`).
- `LNMBackendChannelAccessor`
  (`backends/LogicalNameMapping/include/LNMBackendChannelAccessor.h`).
- `LNMBackendBitAccessor`
  (`backends/LogicalNameMapping/include/LNMBackendBitAccessor.h`).

All follow the same replace / consider-but-keep / forward path structure ending
in an unconditional `setExceptionBackend()`, whose forwarding invocation is
redundant.

Current behaviour, all listed classes:

- The implementation has three paths: replace the target; consider replacement
  but leave the target unchanged; or forward the call to the target.
- All paths end with an unconditional
  `setExceptionBackend(this->_exceptionBackend)` on the target.
- `LNMBackendChannelAccessor` targets its member `_accessor` and has no
  `_target != newElement` guard (its replacement branch re-assigns the same
  accessor); `LNMBackendBitAccessor` creates its new target via
  `detail::createCopyDecorator()`.
- `setExceptionBackend()` recurses into `_target`, descending the whole chain.
- `TransferGroup::addAccessorImpl()` calls `replaceTransferElement()` for all
  combinations of high-level and internal elements, so the unconditional call
  fires for every combination.

Design:

- Keep the propagation in the replacement path: the new target (a copy
  decorator for `NDRegisterAccessorDecorator`/`LNMBackendBitAccessor`, or the
  incoming accessor assigned directly for `BitRangeAccessorDecorator`,
  `SubArrayAccessorDecorator` and `LNMBackendChannelAccessor`) may carry a
  different backend, so this decorator's backend must be re-applied.
- Keep it in the no-replacement path (`_target == newElement`): no recursion
  occurs there, so this is the only propagation.
- Drop it from the forwarding path: the forwarded
  `_target->replaceTransferElement()` performs its own propagation.
- Implemented as, per affected class: propagate inside the
  `casted && mayReplaceOther` branch (covering both the replacement and the
  `_target == newElement` no-replacement case), otherwise forward to
  `_target->replaceTransferElement()` without propagating.
- `setExceptionBackend()` is left unchanged; it stays a pointer assignment plus
  recursion.

Excluded: the other `replaceTransferElement()` implementations do not have the
forward-then-unconditional-`setExceptionBackend()` pattern, so the redundancy
above does not apply to them. `NumericAddressedBackendRegisterAccessor`
(`src/NumericAddressedBackendRegisterAccessor.cc`) and
`NumericAddressedBackendASCIIAccessor` perform a replacement and end in an
unconditional `setExceptionBackend(...)` but never forward; the cost of
`NumericAddressedBackendRegisterAccessor` is tracked separately. The no-op
implementations (empty `replaceTransferElement()`) are `SubdeviceRegisterAccessor`,
`SubdeviceRegisterWindowAccessor`, `DoubleBufferAccessor`,
`LNMDoubleBufferPlugin`, `NumericAddressedBackendMuxedRegisterAccessor`,
`AsyncNDRegisterAccessor`, `LNMBackendVariableAccessor` and
`NumericAddressedLowLevelTransferElement`. `TransferElementAbstractor`
(`src/TransferElementAbstractor.cc`) forwards to its `_impl` but never
propagates the exception backend, so it is unaffected.

### Alternatives considered

- Early-out in `setExceptionBackend()` when the backend pointer is unchanged:
  removes the descent cost of repeated identical calls but leaves the call count
  in place; not chosen because it does not remove the redundancy at its source.
- Also restructuring `NumericAddressedBackendRegisterAccessor`: it does not
  forward, so there is no redundant propagation to remove; a different change
  (e.g. avoiding repeated calls per TransferGroup combination) would be needed
  and is out of scope here.
- Passing the backend as `const&`: `TransferElement::setExceptionBackend()` and
  all overrides take the `shared_ptr` by value; changing the signature touches
  the whole hierarchy and buys only the per-hop copy, not the call count;
  deferred.

## Test plan

- Existing TransferGroup and decorator unit tests pass unchanged.
- Unit test: for every affected class (`NDRegisterAccessorDecorator`,
  `BitRangeAccessorDecorator`, `SubArrayAccessorDecorator`,
  `LNMBackendChannelAccessor`, `LNMBackendBitAccessor`), a forwarded call does
  not invoke `setExceptionBackend()` on the target.
- Unit test: after a replacement, the accessor chain reports the same exception
  backend as before the change.
- Regression: backend exceptions are still wrapped and propagated identically.

## Deferred issue

- Specifications incompleteness: the affected components listed only
  `NDRegisterAccessorDecorator` and `BitRangeAccessorDecorator`, but the same
  forward-then-unconditional-`setExceptionBackend()` pattern also exists in
  `SubArrayAccessorDecorator`, `LNMBackendChannelAccessor` and
  `LNMBackendBitAccessor`; implementing only the two listed classes would not
  satisfy the generally phrased requirement. Scope expanded to all forwarding
  implementations.
- Specifications rationale was inaccurate: the "copy decorator takes its
  exception backend via `initFromTarget()`" reason applied only to
  `NDRegisterAccessorDecorator`; the other classes assign the incoming accessor
  directly, so the replacement path still needs propagation because the new
  target may carry a different backend.
- The "Excluded" note named only `NumericAddressedBackendRegisterAccessor` and
  misclassified `SubdeviceRegisterAccessor`/`SubdeviceRegisterWindowAccessor`
  as non-forwarding though their `replaceTransferElement()` bodies are empty
  no-ops, so the affected/unaffected boundary was unclear. The categories are
  now: replacement-but-no-forward (`NumericAddressedBackendRegisterAccessor`,
  `NumericAddressedBackendASCIIAccessor`), the no-op implementations
  (`SubdeviceRegisterAccessor`, `SubdeviceRegisterWindowAccessor`,
  `DoubleBufferAccessor`, `LNMDoubleBufferPlugin`,
  `NumericAddressedBackendMuxedRegisterAccessor`, `AsyncNDRegisterAccessor`,
  `LNMBackendVariableAccessor`, `NumericAddressedLowLevelTransferElement`), and
  the forwarding-but-non-propagating `TransferElementAbstractor`.
