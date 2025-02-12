// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "async/DataConsistencyRealm.h"

#include <cassert>

namespace ChimeraTK::async {

  /********************************************************************************************************************/

  ChimeraTK::VersionNumber DataConsistencyRealm::getVersion(const DataConsistencyKey& eventId) {
    /*
    // event id will be 0 e.g. if value still comes from the config file (even values changed through the panel will
    // have some macro pulse number attached). See spec B.1.1.1.
    if(eventId.to_int() == 0) {
      return ChimeraTK::VersionNumber{nullptr};
    }
    */

    // acquire lock
    std::lock_guard<decltype(_mapMutex)> lock{_mapMutex};

    // check if entry already exists
    if(_eventIdToVersionMap.find(eventId) != _eventIdToVersionMap.end()) {
      return _eventIdToVersionMap[eventId];
    }

    // special case: map is empty right now, we need to make the first entries.
    if(_eventIdToVersionMap.size() == 0) {
      auto nElementsToInsert = std::min(DataConsistencyKey::BaseType(eventId), maxSizeEventIdMap);
      for(auto eid = DataConsistencyKey(eventId - (nElementsToInsert - 1)); eid <= eventId; ++eid) {
        _eventIdToVersionMap.emplace(eid, VersionNumber{});
      }
      return _eventIdToVersionMap[eventId];
    }

    // check if eventId too old (cf. B.1.1.4.2)
    if(eventId < _eventIdToVersionMap.begin()->first) {
      return ChimeraTK::VersionNumber{nullptr};
    }

    // determine how many elements to insert (according to B.1.1.3.1 we must not leave any gaps)
    auto lastEventId = _eventIdToVersionMap.rbegin()->first;
    auto nElementsToInsert = std::min((eventId - lastEventId), maxSizeEventIdMap);

    // delete old entries to keep map size below maximum (cf. B.1.1.4.1)
    auto nElementsToKeep = maxSizeEventIdMap - nElementsToInsert;
    while(_eventIdToVersionMap.size() > nElementsToKeep) {
      _eventIdToVersionMap.erase(_eventIdToVersionMap.begin());
    }

    // insert new entries
    for(auto eid = DataConsistencyKey(eventId - (nElementsToInsert - 1)); eid <= eventId; ++eid) {
      _eventIdToVersionMap.emplace(eid, VersionNumber{});
    }
    assert(_eventIdToVersionMap.find(eventId) != _eventIdToVersionMap.end());

    // return the requested entry
    return _eventIdToVersionMap[eventId];
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK::async