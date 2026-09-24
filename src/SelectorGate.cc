// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SelectorGate.h"
#include "NumericAddressedBackend.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  void SelectorGate::replace(const boost::shared_ptr<NumericAddressedBackend>& backend, const SelectedBy& selectedBy,
      bool forceFirstFaulty) {
    _accessor.replace(backend->template getSyncRegisterAccessor<int64_t>(selectedBy.regPath, 0, 0, {}));
    _expectedValue = selectedBy.val;
    _forceFirstFaulty = forceFirstFaulty;
    _firstCheck = true;
    _matches = !_forceFirstFaulty;
  }

  /********************************************************************************************************************/

  bool SelectorGate::check() {
    if(_accessor.get() == nullptr) {
      return true;
    }
    _accessor.get()->read();
    _matches = static_cast<int64_t>(_accessor.get()->accessData(0)) == _expectedValue;
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
