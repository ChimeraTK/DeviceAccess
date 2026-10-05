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
    _cacheValid = true;
    return _cachedGateOpen;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void SelectedByDecorator<UserType>::doReadTransferSynchronously() {
    if(!_selectorAccessor) {
      NDRegisterAccessorDecorator<UserType>::doReadTransferSynchronously();
      return;
    }
    // Polled semantics: only perform the physical read of the wrapped accessor when the gate is open.
    // Reuse an earlier gate decision from this cycle (isGateOpen()/check()) if present, otherwise read the
    // selector ourselves; cache the result so doPostRead() does not read the selector again.
    if(!_cacheValid) {
      _selectorAccessor->read();
      _cachedGateOpen = evaluate();
      _cacheValid = true;
    }
    if(_cachedGateOpen) {
      NDRegisterAccessorDecorator<UserType>::doReadTransferSynchronously();
    }
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void SelectedByDecorator<UserType>::doPostRead(TransferType type, bool hasNewData) {
    if(!_selectorAccessor) {
      NDRegisterAccessorDecorator<UserType>::doPostRead(type, hasNewData);
      return;
    }
    // Reuse the gate decision made earlier in this same transfer cycle (by the TransferGroup's
    // isGateOpen() or a check()), which already read the selector and decided whether the data element
    // was transferred. This avoids reading the selector register a second time in the group path. In a
    // standalone read (no group, no check()) no cache is present, so read the selector here.
    if(!_cacheValid) {
      _selectorAccessor->read();
      _cachedGateOpen = evaluate();
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

  template<typename UserType>
  std::vector<boost::shared_ptr<TransferElement>> SelectedByDecorator<UserType>::getGatedElements() {
    return _target->getHardwareAccessingElements();
  }

  /********************************************************************************************************************/

  template<typename UserType>
  bool SelectedByDecorator<UserType>::isGateOpen() {
    if(!_selectorAccessor) {
      return true;
    }
    // Read the selector register to reflect the current selector value before evaluating the gate, and
    // cache the decision so the group's doPostRead() for this cycle does not read the selector again.
    _selectorAccessor->read();
    _cachedGateOpen = evaluate();
    _cacheValid = true;
    return _cachedGateOpen;
  }

  /********************************************************************************************************************/

  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(SelectedByDecorator);

  /********************************************************************************************************************/

} // namespace ChimeraTK
