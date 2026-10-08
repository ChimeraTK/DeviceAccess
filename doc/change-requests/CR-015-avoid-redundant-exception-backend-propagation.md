# CR-015: Avoid redundant exception backend propagation in replaceTransferElement

Synopsis: `replaceTransferElement()` propagates the exception backend to its
target after the replacement step even when nothing was replaced. Where the call
is forwarded to the target, that target already propagates, so the second
propagation is redundant.

Status: IN PROGRESS

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

Current behaviour, both classes:

- The implementation has three paths: replace the target; consider replacement
  but leave the target unchanged; or forward the call to the target.
- All paths end with an unconditional
  `_target->setExceptionBackend(this->_exceptionBackend)`.
- `setExceptionBackend()` recurses into `_target`, descending the whole chain.
- `TransferGroup::addAccessorImpl()` calls `replaceTransferElement()` for all
  combinations of high-level and internal elements, so the unconditional call
  fires for every combination.

Design:

- Keep the propagation in the replacement path: a freshly created copy decorator
  takes its exception backend from the new target via `initFromTarget()`, so
  this decorator's backend must be re-applied.
- Keep it in the no-replacement path (`_target == newElement`): no recursion
  occurs there, so this is the only propagation.
- Drop it from the forwarding path: the forwarded
  `_target->replaceTransferElement()` performs its own propagation.
- Implemented as: propagate inside the `casted && mayReplaceOther` branch,
  otherwise forward only.
- `setExceptionBackend()` is left unchanged; it stays a pointer assignment plus
  recursion.

Excluded: `NumericAddressedBackendRegisterAccessor::replaceTransferElement()`
(`src/NumericAddressedBackendRegisterAccessor.cc`) ends with an unconditional
`_rawAccessor->setExceptionBackend(...)` but has no forwarding path, so the
redundancy above does not apply. Its cost is tracked separately.

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
- Unit test: a decorator whose target is not replaced because the call is
  forwarded does not propagate the exception backend itself.
- Unit test: after a replacement, the accessor chain reports the same exception
  backend as before the change.
- Regression: backend exceptions are still wrapped and propagated identically.
