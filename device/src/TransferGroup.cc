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

    // Store the accessor itself in the high-level list
    highLevelElements.push_back(accessor.getHighLevelImplElement());
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
    // We go backwards through the list, since the latest element is the recently added accessor, for which we still
    // have the original pimpl frontend (variable "accessor"). In cases were we need to replace the entire accessor
    // implementation we need to do this on the pimpl frontent instead of the implementation stored in the
    // highLevelElements list.
    std::vector< std::vector< boost::shared_ptr<TransferElement> >::iterator > eraseList;
    for(int i=elements.size()-1; i>0; --i) {
      for(int k=i-1; k>=0; --k) {
        if(elements[i]->isSameRegister(elements[k])) {
          // replace the TransferElement inside any high-level accessor with the merged version
          accessor.replaceTransferElement(elements[k]);  // might need to replace the implementation of the pimpl accessor frontend
          for(auto &m : highLevelElements) {
            m->replaceTransferElement(elements[k]);
          }
          eraseList.push_back(elements.begin() + i); // store iterator to element to be deleted
          break;
        }
      }
    }

    // Erase the duplicates (cannot do this inside the loop since it would invalidate the iterators)
    for(auto &e : eraseList) elements.erase(e);
    
    // remove elements which are on both the highLevelElements and the elements lists from the highLevelElements, since
    // we should not run postRead() resp. preWrite()/postWrite() on them
    for(auto &elem : elements) {
      bool inHLE = false;
      std::vector< boost::shared_ptr<TransferElement> >::iterator deleteIt;
      for(auto hle = highLevelElements.begin(); hle != highLevelElements.end(); ++hle) {
        if(*hle == elem) {    // this is comparing the shared pointers
          inHLE = true;
         deleteIt = hle;
        }
      }
      if(inHLE) {
        highLevelElements.erase(deleteIt);
        elem->isInTransferGroup = false;  // only high level elements should count as being in the group
      }
    }

    // Update read-only flag
    if(accessor.isReadOnly()) readOnly = true;
  }

} /* namespace mtca4u */
