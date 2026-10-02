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
  SelectedByDecorator<UserType>::SelectedByDecorator(const boost::shared_ptr<NDRegisterAccessor<UserType>>& target,
      const boost::shared_ptr<ScalarRegisterAccessor<int64_t>>& selectorAccessor, int64_t expectedValue)
  : NDRegisterAccessorDecorator<UserType>(target), _selectorAccessor(selectorAccessor) {
    configure(expectedValue);
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
  bool SelectedByDecorator<UserType>::check() {
    if(!_selectorAccessor) {
      return true;
    }
    // Read the selector register so the returned value reflects the current selector state.
    _selectorAccessor->read();
    return evaluate();
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void SelectedByDecorator<UserType>::doPostRead(TransferType type, bool hasNewData) {
    if(!_selectorAccessor) {
      NDRegisterAccessorDecorator<UserType>::doPostRead(type, hasNewData);
      return;
    }
    // Read the selector register so the gating decision reflects the current selector state. When this
    // decorator is part of a TransferGroup the gate has already been evaluated via isGateOpen() in the
    // group's read(); reading again here is slightly redundant but keeps standalone reads correct and is
    // cheap (one narrow synchronous register read).
    _selectorAccessor->read();
    if(!evaluate()) {
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
    // Read the selector register to reflect the current selector value before evaluating the gate.
    _selectorAccessor->read();
    return evaluate();
  }

  /********************************************************************************************************************/

  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(SelectedByDecorator);

  /********************************************************************************************************************/

} // namespace ChimeraTK
