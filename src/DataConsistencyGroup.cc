// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DataConsistencyGroup.h"

#include "HistorizedMatcher.h"
#include "SimpleMatcher.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  void DataConsistencyGroup::initMatcher() {
    if(_mode == MatchingMode::historized) {
      _hImpl = std::make_unique<HistorizedMatcher>(this);
    }
    else {
      _sImpl = std::make_unique<SimpleMatcher>(this);
    }
  };

  /********************************************************************************************************************/

  DataConsistencyGroup::DataConsistencyGroup(MatchingMode mode) : _mode(mode) {
    initMatcher();
  }

  // this line is required since otherwise compiler fails to generate destructor for std::unique_ptr<X> where X
  // is incomplete type xxxMatcher
  DataConsistencyGroup::~DataConsistencyGroup() = default;

  /********************************************************************************************************************/

  DataConsistencyGroup::DataConsistencyGroup(std::initializer_list<TransferElementAbstractor> list) {
    initMatcher();

    for(const auto& element : list) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
      add(element);
#pragma GCC diagnostic pop
    }
  }

  /********************************************************************************************************************/

  DataConsistencyGroup::DataConsistencyGroup(std::initializer_list<boost::shared_ptr<TransferElement>> list) {
    initMatcher();

    for(const auto& element : list) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
      add(element);
#pragma GCC diagnostic pop
    }
  }

  /********************************************************************************************************************/

  DataConsistencyGroup::DataConsistencyGroup(
      std::initializer_list<std::reference_wrapper<TransferElementAbstractor>> list, MatchingMode mode,
      unsigned histLen)
  : _mode(mode) {
    initMatcher();

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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    add(TransferElementAbstractor(std::move(element)));
#pragma GCC diagnostic pop
  }

  /********************************************************************************************************************/

  void DataConsistencyGroup::add(TransferElementAbstractor& acc, unsigned histLen) {
    if(_mode == MatchingMode::historized) {
      _hImpl->add(acc, histLen);
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
        return true;
      case MatchingMode::exact:
        return _sImpl->update(transferElementID);
      case MatchingMode::historized:
        // no need to call HistorizedMatcher::update; it would return true always
        return true;
    }
    assert(false);
    return false;
  }

  /********************************************************************************************************************/

  void DataConsistencyGroup::setMatchingMode(MatchingMode newMode) {
    // we keep the method for compatibility but allow it only for SimpleMatcher
    // With HistorizedMatcher, it won't work since we have no chance finding all accessors that would need to be
    // decorated.
    if(_mode == MatchingMode::historized) {
      throw ChimeraTK::logic_error("setMatchingMode is disallowed once MatchingMode::historized is set");
    }
    _mode = newMode;
  }

  /********************************************************************************************************************/

  bool DataConsistencyGroup::isConsistent() const {
    switch(_mode) {
      case MatchingMode::none:
        // if matching mode is none, always return true
        return true;
      case MatchingMode::exact:
        return _sImpl->isConsistent();
      case MatchingMode::historized:
        // no need to call HistorizedMatcher::findMatch; it would return true
        return true;
    }
    assert(false);
    return false;
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
