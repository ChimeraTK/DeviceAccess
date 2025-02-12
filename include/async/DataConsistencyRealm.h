// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "../VersionNumber.h"
#include "DataConsistencyKey.h"

#include <boost/circular_buffer.hpp>

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
    constexpr static DataConsistencyKey::BaseType maxSizeEventIdMap = 2000;

    // mutex for both _versionHistory and _latestKey
    std::mutex _mapMutex;
    boost::circular_buffer<VersionNumber> _versionHistory{maxSizeEventIdMap};
    DataConsistencyKey _latestKey{0};
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK::async
