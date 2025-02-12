// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "../VersionNumber.h"
#include "DataConsistencyKey.h"

#include <map>
#include <mutex>

namespace ChimeraTK::async {

  class DataConsistencyRealmStore;

  /********************************************************************************************************************/

  class DataConsistencyRealm {
   public:
    DataConsistencyRealm() = default;
    ~DataConsistencyRealm() = default;

    ChimeraTK::VersionNumber getVersion(const DataConsistencyKey& eventId);

    DataConsistencyRealm(const DataConsistencyRealm&) = delete;
    DataConsistencyRealm& operator=(const DataConsistencyRealm&) = delete;

   private:
    std::mutex _mapMutex;
    std::map<DataConsistencyKey, ChimeraTK::VersionNumber> _eventIdToVersionMap;

    constexpr static DataConsistencyKey::BaseType maxSizeEventIdMap = 2000;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK::async