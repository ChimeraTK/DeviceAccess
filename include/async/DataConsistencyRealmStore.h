// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "DataConsistencyRealm.h"

#include <map>
#include <mutex>
#include <string>

namespace ChimeraTK::async {

  /********************************************************************************************************************/

  class DataConsistencyRealmStore {
   public:
    static DataConsistencyRealmStore& getInstance() {
      static DataConsistencyRealmStore instance;
      return instance;
    }

    std::shared_ptr<DataConsistencyRealm> getRealm(const std::string& realmName);

   private:
    DataConsistencyRealmStore() = default;
    ~DataConsistencyRealmStore() = default;

    std::mutex _mapMutex;
    std::map<std::string, std::weak_ptr<DataConsistencyRealm>> _realmMap;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK::async
