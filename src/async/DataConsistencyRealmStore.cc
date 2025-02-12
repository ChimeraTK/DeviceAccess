// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "async/DataConsistencyRealmStore.h"

namespace ChimeraTK::async {

  /********************************************************************************************************************/

  std::shared_ptr<DataConsistencyRealm> DataConsistencyRealmStore::getRealm(const std::string& realmName) {
    std::lock_guard<std::mutex> lock(_mapMutex);

    auto it = _realmMap.find(realmName);
    if(it != _realmMap.end()) {
      if(auto realm = it->second.lock()) {
        return realm;
      }
    }

    auto realm = std::make_shared<DataConsistencyRealm>();
    _realmMap[realmName] = realm;
    return realm;
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK::async
