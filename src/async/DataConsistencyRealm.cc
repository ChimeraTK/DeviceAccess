// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "async/DataConsistencyRealm.h"

namespace ChimeraTK::async {

  /********************************************************************************************************************/

  ChimeraTK::VersionNumber DataConsistencyRealm::getVersion(const DataConsistencyKey& eventId) {
    // event id will be 0 e.g. if value still comes from the config file (even values changed through the panel will
    // have some macro pulse number attached). See spec B.3.1.1.
    if(DataConsistencyKey::BaseType(eventId) == 0) {
      return ChimeraTK::VersionNumber{nullptr};
    }

    // acquire lock
    auto lock = std::lock_guard(_mapMutex);

    // check if entry already exists
    // Note: the second part of the condition "eventId > _latestKey - maxSizeEventIdMap" has been rewritten like this
    // to prevent negative overflows.
    if(eventId <= _latestKey && eventId + maxSizeEventIdMap > _latestKey) {
      auto index = _versionHistory.size() - (_latestKey - eventId) - 1;
      return _versionHistory[index];
    }

    // special case: map is empty right now, we need to make the first entries.
    if(_versionHistory.empty()) {
      auto nElementsToInsert = std::min(DataConsistencyKey::BaseType(eventId), maxSizeEventIdMap);
      for(size_t i = 0; i < nElementsToInsert; ++i) {
        _versionHistory.push_back({});
      }
      _latestKey = eventId;

      return _versionHistory.back();
    }

    // check if eventId too old (cf. spec B.3.1.3.1.2)
    // Note: the condition "eventId <= _latestKey - maxSizeEventIdMap" is rewritten like this to prevent
    // negative overflows
    if(eventId + maxSizeEventIdMap <= _latestKey) {
      return ChimeraTK::VersionNumber{nullptr};
    }

    // determine how many elements to insert (according to spec B.3.1.3.2 we must not leave any gaps)
    auto nElementsToInsert = std::min((eventId - _latestKey), maxSizeEventIdMap);

    // insert new entries
    for(size_t i = 0; i < nElementsToInsert; ++i) {
      _versionHistory.push_back(VersionNumber{});
    }
    _latestKey = eventId;

    // return the requested entry
    return _versionHistory.back();
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK::async
