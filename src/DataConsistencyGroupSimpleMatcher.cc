// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DataConsistencyGroupSimpleMatcher.h"

namespace ChimeraTK::DataConsistencyGroupDetail {

  /********************************************************************************************************************/

  bool SimpleMatcher::update(const TransferElementID& transferElementID) {
    auto getVNFromElement = _pushElements.at(transferElementID).getVersionNumber();
    assert(getVNFromElement != VersionNumber{nullptr});
    if(getVNFromElement < _versionNumberToBeConsistentTo) {
      return false;
    }
    if(_versionNumberToBeConsistentTo != getVNFromElement) {
      _versionNumberToBeConsistentTo = getVNFromElement;
      _consistentElements.clear();
    }
    _consistentElements.insert(transferElementID);
    return _consistentElements.size() == _pushElements.size();
  }

  /********************************************************************************************************************/

  bool SimpleMatcher::isConsistent() const {
    return _consistentElements.size() == _pushElements.size();
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK::DataConsistencyGroupDetail
