// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DataConsistencyGroup.h"

#include "HistorizedMatcher.h"
#include "SimpleMatcher.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  DataConsistencyGroup::DataConsistencyGroup(MatchingMode mode) : _mode(mode) {
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

  DataConsistencyGroup::DataConsistencyGroup(
      std::initializer_list<std::reference_wrapper<TransferElementAbstractor>> list, MatchingMode mode,
      unsigned histLen) {
    setMatchingMode(mode); // must be called before add since otherwise decoration won't apply
    for(const auto& element : list) {
      add(element, histLen);
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

  void DataConsistencyGroup::add(TransferElementAbstractor& acc, unsigned int histLen) {
    // TODO instantiate hImpl
    if(_mode == MatchingMode::historized) {
      hImpl->add(acc, histLen);
    }
  }

  /********************************************************************************************************************/

  bool DataConsistencyGroup::update(const TransferElementID& transferElementID) {
    // ignore transferElementID not in DataConsistencyGroup
    if(_pushElements.find(transferElementID) == _pushElements.end()) {
      return false;
    }
    switch(_mode) {
      case MatchingMode::none:
        // if matching mode is none, always return true
        return true;
      case MatchingMode::exact:
        return sImpl->update(transferElementID);
      case MatchingMode::historized:
        // no need to call HistorizedMatcher::update; it would return true
        return true;
    }
  }

  /********************************************************************************************************************/

  void DataConsistencyGroup::setMatchingMode(MatchingMode newMode) {
    // TODO add checks and apply decoration for historized?
    // I'm not sure that we switch to historized at all - problem being the
    // abstractors not saved by us!
    _mode = newMode;
  }

  /********************************************************************************************************************/

  bool DataConsistencyGroup::isConsistent() const {
    switch(_mode) {
      case MatchingMode::none:
        // if matching mode is none, always return true
        return true;
      case MatchingMode::exact:
        return sImpl->isConsistent();
      case MatchingMode::historized:
        // no need to call HistorizedMatcher::findMatch; it would return true
        return true;
    }
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
