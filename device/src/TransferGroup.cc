/*
 * TransferGroup.cc
 *
 *  Created on: Feb 11, 2016
 *      Author: Martin Hierholzer
 */

#include "TransferGroup.h"
#include "BufferingRegisterAccessor.h"
#include "TwoDRegisterAccessor.h"

namespace mtca4u {

  void TransferGroup::read() {
    for(unsigned int i=0; i<elements.size(); i++) {
      elements[i]->read();
    }
  }

  /*********************************************************************************************************************/

  void TransferGroup::write() {
    for(unsigned int i=0; i<elements.size(); i++) {
      elements[i]->write();
    }
  }

  /*********************************************************************************************************************/

  void TransferGroup::uniqueInsert(std::vector< boost::shared_ptr<TransferElement>* > &newElements) {

    // iterate over all new elements
    for(unsigned int i=0; i<newElements.size(); i++) {

      // iterate over all elements already in the list and check if element is already in there
      bool foundDuplicate = false;
      for(unsigned int k=0; k<elements.size(); k++) {
        if(*(*(newElements[i])) == *(elements[k])) {  // (pointer of shared pointer vs shared pointer...)

          // update the shared pointer in the newElements list to point to the found duplicate
          *(newElements[i]) = elements[k];

          foundDuplicate = true;
          break;
        }
      }

      // if not a duplicate, add to the list
      if(!foundDuplicate) elements.push_back(*(newElements[i]));

    }

  }

} /* namespace mtca4u */
