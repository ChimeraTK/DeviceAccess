/*
 * TransferGroup.cc
 *
 *  Created on: Feb 11, 2016
 *      Author: Martin Hierholzer
 */

#include "TransferGroup.h"
#include "DeviceException.h"
#include "BufferingRegisterAccessor.h"
#include "TwoDRegisterAccessor.h"

namespace mtca4u {

  void TransferGroup::read() {
    for(unsigned int i=0; i<elements.size(); i++) {
      elements[i]->read();
    }
    for(unsigned int i=0; i<highLevelElements.size(); i++) {
      highLevelElements[i]->postRead();
    }
  }

  /*********************************************************************************************************************/

  void TransferGroup::write() {
    if(isReadOnly()) {
      throw DeviceException("TransferGroup::write() called, but the TransferGroup is read-only.",
          DeviceException::REGISTER_IS_READ_ONLY);
    }
    for(unsigned int i=0; i<highLevelElements.size(); i++) {
      highLevelElements[i]->preWrite();
    }
    for(unsigned int i=0; i<elements.size(); i++) {
      elements[i]->write();
    }
    for(unsigned int i=0; i<highLevelElements.size(); i++) {
      highLevelElements[i]->postWrite();
    }
  }

  /*********************************************************************************************************************/

  bool TransferGroup::isReadOnly() {
    return readOnly;
  }

  /*********************************************************************************************************************/

  template<>
  void TransferGroup::addAccessor<TransferElement>(TransferElement &accessor) {

    // check if accessor is already in a transfer group
    if(accessor.getHighLevelImplElement()->isInTransferGroup) {
      throw DeviceException("The given accessor is already in a TransferGroup and cannot be added to another.",
          DeviceException::WRONG_PARAMETER);
    }
    accessor.getHighLevelImplElement()->isInTransferGroup = true;

    // Iterate over all new hardware-accessing elements and add them to the list
    auto newElements = accessor.getHardwareAccessingElements();
    for(unsigned int i=0; i<newElements.size(); i++) {
      elements.push_back(newElements[i]);
    }

    // Iterate over all hardware-accessing elements and merge any potential duplicates
    // Note: This cannot be done only for the newly added elements, since e.g. in NumericAddressedBackend the new
    // accessor might fill the gap (in the address space) between to already added accessors, in which case also the
    // already added accessors must be merged.
    std::vector< std::vector< boost::shared_ptr<TransferElement> >::iterator > eraseList;
    for(auto i = elements.begin(); i != elements.end(); ++i) {
      for(auto k = i; k != elements.end(); ++k) {
        if(k == i) continue;
        if((*i)->isSameRegister(*k)) {
          // replace the TransferElement inside any high-level accessor with the merged version
          for(auto &m : highLevelElements) {
            m->replaceTransferElement(*k);
          }
          eraseList.push_back(i);
          break;
        }
      }
    }

    // Erase the duplicates (cannot do this inside the loop since it would invalidate the iterators)
    for(auto &e : eraseList) elements.erase(e);

    // Update read-only flag
    if(accessor.isReadOnly()) readOnly = true;

    // Store the accessor itself in the high-level list
    highLevelElements.push_back(accessor.getHighLevelImplElement());
  }

} /* namespace mtca4u */
