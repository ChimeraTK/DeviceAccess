// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DataConsistencyGroup.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  DataConsistencyGroup::DataConsistencyGroup() = default;

  /********************************************************************************************************************/

  DataConsistencyGroup::DataConsistencyGroup(std::initializer_list<TransferElementAbstractor> list) {
    for(const auto& element : list) add(element);
  }

  /********************************************************************************************************************/

  DataConsistencyGroup::DataConsistencyGroup(std::initializer_list<boost::shared_ptr<TransferElement>> list) {
    for(const auto& element : list) add(element);
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
    push_elements[element.getId()] = element;
  }

  /********************************************************************************************************************/

  void DataConsistencyGroup::add(boost::shared_ptr<TransferElement> element) {
    add(TransferElementAbstractor(std::move(element)));
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
