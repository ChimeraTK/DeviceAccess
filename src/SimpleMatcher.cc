// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SimpleMatcher.h"

#include "DataConsistencyGroup.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  bool SimpleMatcher::update(const TransferElementID& transferElementID) {
    using MatchingMode = DataConsistencyGroup::MatchingMode;
    assert(_dg->_mode == MatchingMode::exact);

    auto getVNFromElement = _dg->_pushElements[transferElementID].getVersionNumber();
    assert(getVNFromElement != VersionNumber{nullptr});
    if(getVNFromElement < _dg->versionNumberToBeConsistentTo) {
      return false;
    }
    if(_dg->versionNumberToBeConsistentTo != getVNFromElement) {
      _dg->versionNumberToBeConsistentTo = getVNFromElement;
      _consistentElements.clear();
    }
    _consistentElements.insert(transferElementID);
    return _consistentElements.size() == _dg->_pushElements.size();
  }

  bool SimpleMatcher::isConsistent() const {
    return _consistentElements.size() == _dg->_pushElements.size();
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
