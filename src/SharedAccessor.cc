// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SharedAccessor.h"

namespace ChimeraTK::detail {

  /********************************************************************************************************************/

  void SharedAccessors::combineTransferSharedStates(TransferElementID oldId, TransferElementID newId) {
    std::lock_guard<std::mutex> l(_mapMutex); // protect against concurrent map insertion via the [] operator
    auto oldIter = _transferSharedStates.find(oldId);
    assert(oldIter != _transferSharedStates.end());
    auto newIter = _transferSharedStates.find(newId);
    assert(newIter != _transferSharedStates.end());

    // Shift instanceCount from old to new. When old is no longer used after the shifting,
    // drop the old entry
    assert(oldIter->second.instanceCount >= 1);
    --(oldIter->second.instanceCount);
    ++(newIter->second.instanceCount);
    if(oldIter->second.instanceCount == 0) {
      _transferSharedStates.erase(oldIter);
    }
  }

  /********************************************************************************************************************/

  void SharedAccessors::addTransferElement(TransferElementID id) {
    std::lock_guard<std::mutex> l(_mapMutex); // protect against concurrent map insertion via the [] operator
    assert(_transferSharedStates.find(id) == _transferSharedStates.end()); // must not be in the map yet
    _transferSharedStates[id].instanceCount = 1;
  }

  /********************************************************************************************************************/

  void SharedAccessors::removeTransferElement(TransferElementID id) {
    std::lock_guard<std::mutex> l(_mapMutex); // protect against concurrent map insertion via the [] operator
    auto iter = _transferSharedStates.find(id);
    assert(iter != _transferSharedStates.end()); // must be in the map
    assert(iter->second.instanceCount >= 1);
    --(iter->second.instanceCount);
    if(iter->second.instanceCount == 0) {
      _transferSharedStates.erase(iter);
    }
  }

  /********************************************************************************************************************/

  size_t SharedAccessors::instanceCount(TransferElementID id) {
    std::lock_guard<std::mutex> l(_mapMutex); // protect against concurrent map insertion via the [] operator
    auto iter = _transferSharedStates.find(id);
    if(iter == _transferSharedStates.end()) {
      return 0;
    }
    return iter->second.instanceCount;
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK::detail
