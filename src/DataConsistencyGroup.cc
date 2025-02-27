// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DataConsistencyGroup.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  DataConsistencyGroup::DataConsistencyGroup() {
    // TODO replace by smart pointer, or pointer to base matcher?
    sImpl = new SimpleMatcher(this);
  };

  /********************************************************************************************************************/

  DataConsistencyGroup::DataConsistencyGroup(std::initializer_list<TransferElementAbstractor> list) {
    for(const auto& element : list) {
      add(element);
    }
  }

  /********************************************************************************************************************/

  DataConsistencyGroup::DataConsistencyGroup(std::initializer_list<boost::shared_ptr<TransferElement>> list) {
    for(const auto& element : list) {
      add(element);
    }
  }

  /********************************************************************************************************************/

  void DataConsistencyGroup::add(const TransferElementAbstractor& element) {
    if(!element.isReadable()) {
      throw ChimeraTK::logic_error(
          "Cannot add non-readable accessor for register " + element.getName() + " to DataConsistencyGroup.");
    }
    if(!element.getAccessModeFlags().has(AccessMode::wait_for_new_data)) {
      throw ChimeraTK::logic_error(
          "Cannot add poll type accessor for register " + element.getName() + " to DataConsistencyGroup.");
    }
    _pushElements[element.getId()] = element;
  }

  /********************************************************************************************************************/

  void DataConsistencyGroup::add(boost::shared_ptr<TransferElement> element) {
    add(TransferElementAbstractor(std::move(element)));
  }

  /********************************************************************************************************************/

  bool DataConsistencyGroup::update(const TransferElementID& transferElementID) {
    // ignore transferElementID does not belong to DataConsistencyGroup
    if(_pushElements.find(transferElementID) == _pushElements.end()) {
      return false;
    }
    // if matching mode is none, always return true
    if(_mode == MatchingMode::none) {
      return true;
    }

    return sImpl->update(transferElementID);
  }

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
