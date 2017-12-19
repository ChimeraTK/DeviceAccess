/*
 * TransferGroup.cc
 *
 *  Created on: Feb 11, 2016
 *      Author: Martin Hierholzer
 */

#include "TransferGroup.h"
#include "TransferElementAbstractor.h"
#include "DeviceException.h"
#include "NDRegisterAccessorAbstractor.h"
#include "NDRegisterAccessorDecorator.h"
#include "CopyRegisterDecorator.h"

namespace mtca4u {

  /*********************************************************************************************************************/

  void TransferGroup::read() {
    for(auto &elem : highLevelElements) {
      elem->preRead();
    }
    for(auto &elem : lowLevelElements) {
      elem->doReadTransfer();
    }
    for(auto &elem : copyDecorators) {
      elem->postRead();
    }
    for(auto &elem : highLevelElements) {
      elem->postRead();
    }
  }

  /*********************************************************************************************************************/

  void TransferGroup::write() {
    if(isReadOnly()) {
      throw DeviceException("TransferGroup::write() called, but the TransferGroup is read-only.",
          DeviceException::REGISTER_IS_READ_ONLY);
    }
    for(auto &elem : highLevelElements) {
      elem->preWrite();
    }
    for(auto &elem : lowLevelElements) {
      elem->doWriteTransfer();
    }
    for(auto &elem : highLevelElements) {
      elem->postWrite();
    }
  }

  /*********************************************************************************************************************/

  bool TransferGroup::isReadOnly() {
    return readOnly;
  }

  /*********************************************************************************************************************/

  void TransferGroup::addAccessor(TransferElementAbstractor &accessor) {

    // check if accessor is already in a transfer group
    if(accessor.getHighLevelImplElement()->isInTransferGroup) {
      throw DeviceException("The given accessor is already in a TransferGroup and cannot be added to another.",
          DeviceException::WRONG_PARAMETER);
    }

    // set flag on the accessors that it is now in a transfer group
    accessor.getHighLevelImplElement()->isInTransferGroup = true;

    auto highLevelElementsWithNewAccessor = highLevelElements;
    highLevelElementsWithNewAccessor.insert(accessor.getHighLevelImplElement());

    // try replacing all internal elements in all high-level elements
    for(auto &hlElem1 : highLevelElementsWithNewAccessor) {

      auto list = hlElem1->getInternalElements();
      list.push_front(hlElem1);

      for(auto &replacement : list) {
        // try on the abstractor first, to make sure we replace at the highest level if possible
        accessor.replaceTransferElement(replacement);
        // try on all high-level elements already stored in the list
        for(auto &hlElem : highLevelElementsWithNewAccessor) {
          hlElem->replaceTransferElement(replacement);     // note: this does nothing, if the replacement cannot be used by the hlElem!
        }
      }
    }

    // store the accessor in the list of high-level elements
    // this must be done only now, since it may have been replaced during the replacement process
    highLevelElements.insert(accessor.getHighLevelImplElement());

    // update the list of hardware-accessing elements, since we might just have made some of them redundant
    // since we are using a set to store the elements, duplicates are intrinsically avoided.
    lowLevelElements.clear();
    for(auto &hlElem : highLevelElements) {
      for(auto &hwElem : hlElem->getHardwareAccessingElements()) lowLevelElements.insert(hwElem);
    }

    // update the list of CopyRegisterDecorators
    copyDecorators.clear();
    for(auto &hlElem : highLevelElements) {
      if(boost::dynamic_pointer_cast<ChimeraTK::CopyRegisterDecoratorTrait>(hlElem) != nullptr) {
        copyDecorators.insert(hlElem);
      }
      for(auto &hwElem : hlElem->getInternalElements()) {
        if(boost::dynamic_pointer_cast<ChimeraTK::CopyRegisterDecoratorTrait>(hwElem) != nullptr) {
          copyDecorators.insert(hwElem);
        }
      }
    }

    // Update read-only flag
    if(accessor.isReadOnly()) readOnly = true;
  }

  /*********************************************************************************************************************/

  namespace detail {
    /// just used in TransferGroup::addAccessor(const boost::shared_ptr<TransferElement> &accessor)
    struct TransferGroupTransferElementAbstractor : TransferElementAbstractor {
      TransferGroupTransferElementAbstractor(boost::shared_ptr< TransferElement > impl)
      : TransferElementAbstractor(impl) {}

      void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) override {
        _implUntyped->replaceTransferElement(newElement);
      };
    };
  }

  /*********************************************************************************************************************/

  void TransferGroup::addAccessor(const boost::shared_ptr<TransferElement> &accessor) {
    /// @todo implement smarter and more efficient!
    auto x = detail::TransferGroupTransferElementAbstractor(accessor);
    addAccessor(x);
  }

  /*********************************************************************************************************************/

  void TransferGroup::dump() {

    std::cout << "=== Accessors added to this group: " << std::endl;
    for(auto &elem : highLevelElements) {
      std::cout << " - " << elem->getName() << std::endl;
    }
    std::cout << "=== Low-level transfer elements in this group: " << std::endl;
    for(auto &elem : lowLevelElements) {
      std::cout << " - " << elem->getName() << std::endl;
    }
    std::cout << "===" << std::endl;

  }

} /* namespace mtca4u */
