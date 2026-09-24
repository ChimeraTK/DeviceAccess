// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SelectorGate.h"
#include "NumericAddressedBackend.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  void SelectorGate::replace(const boost::shared_ptr<NumericAddressedBackend>& backend, const SelectedBy& selectedBy,
      bool forceFirstFaulty) {
    _accessor = boost::make_shared<ScalarRegisterAccessor<int64_t>>(
        backend->template getSyncRegisterAccessor<int64_t>(selectedBy.regPath, 0, 0, {}));
    _managedExternally = false;
    _expectedValue = selectedBy.val;
    _forceFirstFaulty = forceFirstFaulty;
    _firstCheck = true;
    _matches = !_forceFirstFaulty;
  }

  /********************************************************************************************************************/

  boost::shared_ptr<ScalarRegisterAccessor<int64_t>> SelectorGate::makeSharedAccessor(
      const boost::shared_ptr<NumericAddressedBackend>& backend, const SelectedBy& selectedBy) {
    return boost::make_shared<ScalarRegisterAccessor<int64_t>>(
        backend->template getSyncRegisterAccessor<int64_t>(selectedBy.regPath, 0, 0, {}));
  }

  /********************************************************************************************************************/

  void SelectorGate::attach(const boost::shared_ptr<ScalarRegisterAccessor<int64_t>>& accessor, int64_t expectedValue,
      bool forceFirstFaulty) {
    _accessor = accessor;
    _managedExternally = true;
    _expectedValue = expectedValue;
    _forceFirstFaulty = forceFirstFaulty;
    _firstCheck = true;
    _matches = !_forceFirstFaulty;
  }

  /********************************************************************************************************************/

  bool SelectorGate::check() {
    if(_accessor.get() == nullptr) {
      return true;
    }
    // A TransferGroup-managed gate relies on the accessor having been read as part of the group's
    // read(); issuing our own read here would defeat the deduplication. Self-owned gates read now.
    if(!_managedExternally) {
      _accessor->read();
    }
    _matches = static_cast<int64_t>(*_accessor) == _expectedValue;
    if(_firstCheck) {
      _firstCheck = false;
      // forceFirstFaulty: report faulty until the first matching selector value was observed.
      if(_forceFirstFaulty && !_matches) {
        _matches = false;
      }
    }
    // Once a matching selector value has been seen, subsequent non-matching reads report faulty only.
    return _matches;
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
