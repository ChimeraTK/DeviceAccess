// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DataConsistencyGroup.h"

#include "DataConsistencyGroupHistorizedMatcher.h"
#include "DataConsistencyGroupSimpleMatcher.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  void DataConsistencyGroup::initMatcher() {
    if(_mode == MatchingMode::historized) {
      _impl = std::make_unique<DataConsistencyGroupDetail::HistorizedMatcher>();
    }
    else {
      _impl = std::make_unique<DataConsistencyGroupDetail::SimpleMatcher>();
    }
  };

  /********************************************************************************************************************/

  DataConsistencyGroup::DataConsistencyGroup(MatchingMode mode) : _mode(mode) {
    initMatcher();
  }

  // move constructor required from ApplicationCore
  DataConsistencyGroup::DataConsistencyGroup(DataConsistencyGroup&& other) noexcept = default;
  DataConsistencyGroup& DataConsistencyGroup::operator=(DataConsistencyGroup&& other) noexcept = default;

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
    checkAccess(element);
    if(_mode == MatchingMode::historized) {
      throw ChimeraTK::logic_error(
          "Add function with const-abstractor is deprecated and unsuitable for MatchingMode::historized");
    }
    _impl->getElements()[element.getId()] = element;
  }

  /********************************************************************************************************************/

  void DataConsistencyGroup::add(boost::shared_ptr<TransferElement> element) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    add(TransferElementAbstractor(std::move(element)));
#pragma GCC diagnostic pop
  }

  /********************************************************************************************************************/

  void DataConsistencyGroup::checkAccess(const TransferElementAbstractor& element) {
    if(!element.isReadable()) {
      throw ChimeraTK::logic_error(
          "Cannot add non-readable accessor for register " + element.getName() + " to DataConsistencyGroup.");
    }
    if(!element.getAccessModeFlags().has(AccessMode::wait_for_new_data)) {
      throw ChimeraTK::logic_error(
          "Cannot add poll type accessor for register " + element.getName() + " to DataConsistencyGroup.");
    }
  }

  /********************************************************************************************************************/

  void DataConsistencyGroup::add(TransferElementAbstractor& element, unsigned histLen) {
    checkAccess(element);
    if(_mode == MatchingMode::historized) {
      dynamic_cast<DataConsistencyGroupDetail::HistorizedMatcher*>(_impl.get())->add(element, histLen);
    }
    else {
      _impl->getElements()[element.getId()] = element;
    }
  }

  /********************************************************************************************************************/

  bool DataConsistencyGroup::update(const TransferElementID& transferElementID) {
    // ignore transferElementID not in DataConsistencyGroup
    if(_impl->getElements().find(transferElementID) == _impl->getElements().end()) {
      return false;
    }
    switch(_mode) {
      case MatchingMode::none:
        return true;
      case MatchingMode::exact:
        return dynamic_cast<DataConsistencyGroupDetail::SimpleMatcher*>(_impl.get())->update(transferElementID);
      case MatchingMode::historized:
        // no need to call HistorizedMatcher::checkUpdate here; it would return true always
        // But to allow check of right usage (did user call DataConsistencyGroup::update?), set a flag
        dynamic_cast<DataConsistencyGroupDetail::HistorizedMatcher*>(_impl.get())->updateCalled(transferElementID);
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
    if(_mode == MatchingMode::historized || newMode == MatchingMode::historized) {
      throw ChimeraTK::logic_error("setMatchingMode cannot be used to switch to or from MatchingMode::historized");
    }
    _mode = newMode;
  }

  /********************************************************************************************************************/

  const std::map<TransferElementID, TransferElementAbstractor>& DataConsistencyGroup::getElements() const {
    return _impl->getElements();
  }

  /********************************************************************************************************************/

  const DataConsistencyGroupDetail::MatcherBase& DataConsistencyGroup::getMatcher() {
    return *_impl;
  }

  /********************************************************************************************************************/

  bool DataConsistencyGroup::isConsistent() const {
    switch(_mode) {
      case MatchingMode::none:
        // if matching mode is none, always return true
        return true;
      case MatchingMode::exact:
        return dynamic_cast<DataConsistencyGroupDetail::SimpleMatcher*>(_impl.get())->isConsistent();
      case MatchingMode::historized:
        // no need to call HistorizedMatcher::findMatch; it would return true
        return true;
    }
    assert(false);
    return false;
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
