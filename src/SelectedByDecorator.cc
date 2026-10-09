// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SelectedByDecorator.h"

#include "NumericAddressedBackend.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  template<typename UserType>
  SelectedByDecorator<UserType>::SelectedByDecorator(const boost::shared_ptr<NDRegisterAccessor<UserType>>& target,
      const boost::shared_ptr<NumericAddressedBackend>& backend, const SelectedBy& selectedBy)
  : NDRegisterAccessorDecorator<UserType>(target),
    _selectorAccessor(boost::make_shared<ScalarRegisterAccessor<int64_t>>(
        backend->template getSyncRegisterAccessor<int64_t>(selectedBy.regPath, 0, 0, {}))) {
    configure(selectedBy.val);
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void SelectedByDecorator<UserType>::configure(int64_t expectedValue) {
    _expectedValue = expectedValue;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  bool SelectedByDecorator<UserType>::evaluate() {
    return static_cast<int64_t>(*_selectorAccessor) == _expectedValue;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void SelectedByDecorator<UserType>::doPreRead(TransferType type) {
    // Start a fresh transfer cycle: any gate decision cached by a previous isGateOpen()/check() is no
    // longer valid and must not be reused by the upcoming doPostRead().
    _cacheValid = false;
    NDRegisterAccessorDecorator<UserType>::doPreRead(type);
  }

  /********************************************************************************************************************/

  template<typename UserType>
  bool SelectedByDecorator<UserType>::check() {
    if(!_selectorAccessor) {
      return true;
    }
    // Read the selector register so the returned value reflects the current selector state.
    _selectorAccessor->read();
    _cachedGateOpen = evaluate();
    _lastGateOpen = _cachedGateOpen;
    _cacheValid = true;
    return _cachedGateOpen;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void SelectedByDecorator<UserType>::doReadTransferSynchronously() {
    if(!_selectorAccessor || !_skipWhenUnselected) {
      // Polled path (or no selection gate): always read the wrapped data accessor. The gate is applied
      // afterwards in doPostRead() (validity level).
      NDRegisterAccessorDecorator<UserType>::doReadTransferSynchronously();
      return;
    }
    // Interrupt path with skipping enabled: evaluate the gate before the transfer. Reading the selector
    // here (instead of after) lets us skip the (potentially expensive, shared/muxed) data accessor while
    // unselected. The selector is still read so a selection change is picked up on a later interrupt.
    _selectorAccessor->read();
    _cachedGateOpen = evaluate();
    _lastGateOpen = _cachedGateOpen;
    _cacheValid = true;
    if(_cachedGateOpen) {
      _transferSkipped = false;
      NDRegisterAccessorDecorator<UserType>::doReadTransferSynchronously();
      return;
    }
    // Gate closed: skip the physical read of the wrapped data accessor entirely. doPostRead() will report
    // faulty/not-new without touching the (stale) wrapped buffer.
    _transferSkipped = true;
    this->_dataValidity = DataValidity::faulty;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void SelectedByDecorator<UserType>::doPostRead(TransferType type, bool hasNewData) {
    if(!_selectorAccessor) {
      NDRegisterAccessorDecorator<UserType>::doPostRead(type, hasNewData);
      return;
    }
    if(_transferSkipped) {
      // A skipped transfer: the wrapped data accessor was not read, so its postRead must not be invoked
      // (its buffer is stale). doReadTransferSynchronously() already marked the data faulty; report it as
      // not-new so the consumer does not treat it as fresh data.
      _transferSkipped = false;
      _cacheValid = false;
      return;
    }
    // Reuse the gate decision made earlier in this same transfer cycle by check() (the async wake path) /
    // whether the data element is selected. This avoids reading the selector register a second time. In a
    // polled standalone read / group read (no check()) no cache is present, so read the selector here, after
    // the physical transfer has been performed.
    if(!_cacheValid) {
      _selectorAccessor->read();
      _cachedGateOpen = evaluate();
      _lastGateOpen = _cachedGateOpen;
    }
    _cacheValid = false;
    if(!_cachedGateOpen) {
      // Unselected: report the read as faulty and treat it as not-new. Forwarding with updateDataBuffer
      // false lets a double-buffered target keep its previous buffer (the inactive alternative is not
      // swapped in as fresh data); the base decorator overwrites the validity from the target, so the
      // faulty signal is applied afterwards.
      NDRegisterAccessorDecorator<UserType>::doPostRead(type, false);
      this->_dataValidity = DataValidity::faulty;
      return;
    }
    NDRegisterAccessorDecorator<UserType>::doPostRead(type, hasNewData);
  }

  /********************************************************************************************************************/

  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(SelectedByDecorator);

  /********************************************************************************************************************/

} // namespace ChimeraTK
