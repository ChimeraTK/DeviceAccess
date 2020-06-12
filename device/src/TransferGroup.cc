/*
 * TransferGroup.cc
 *
 *  Created on: Feb 11, 2016
 *      Author: Martin Hierholzer
 */

#include "TransferGroup.h"
#include "CopyRegisterDecorator.h"
#include "Exception.h"
#include "NDRegisterAccessorAbstractor.h"
#include "NDRegisterAccessorDecorator.h"
#include "TransferElement.h"
#include "TransferElementAbstractor.h"
#include <iostream>

namespace ChimeraTK {

  /*********************************************************************************************************************/

  TransferGroup::ExceptionHandlingResult TransferGroup::runPostReads(
      std::set<boost::shared_ptr<TransferElement>>& elements) {
    ExceptionHandlingResult result;

    for(auto& elem : elements) {
      bool updateUserBuffer{true}; // local variable if this element had an exception, to pass on to postRead();
      // check for exceptions on any of the element's low level elements
      for(auto& lowLevelElem : elem->getHardwareAccessingElements()) {
        if(lowLevelElem->_activeException) {
          updateUserBuffer = false;
        }
      }

      result += handlePostExceptions([&] { elem->postRead(TransferType::read, updateUserBuffer); });
    }

    return result;
  }

  template<typename Callable>
  TransferGroup::ExceptionHandlingResult TransferGroup::handlePostExceptions(Callable function) {
    try {
      function();
    }
    catch(ChimeraTK::runtime_error& ex) {
      return ExceptionHandlingResult(true, ex.what());
    }
    catch(ChimeraTK::logic_error& ex) {
      return ExceptionHandlingResult(true, ex.what());
    }
    catch(boost::numeric::bad_numeric_cast& ex) {
      return ExceptionHandlingResult(true, ex.what());
    }
    catch(boost::thread_interrupted&) {
      return ExceptionHandlingResult(true, {}, true); // report that we have seen a thread_interrupted exception
    }
    catch(...) {
      std::cout << "BUG: Wrong exception type thrown in doPostRead() or doPostWrite()!" << std::endl;
      std::terminate();
    }
    return ExceptionHandlingResult(); // result without exceptions
  }

  void TransferGroup::read() {
    // reset exception flags
    for(auto& it : lowLevelElementsAndExceptionFlags) {
      it.second = false;
    }

    for(auto& elem : highLevelElements) {
      elem->preReadAndHandleExceptions(TransferType::read);
      if(elem->_activeException) {
        for(auto& lowLevelElem : elem->getHardwareAccessingElements()) {
          lowLevelElementsAndExceptionFlags[lowLevelElem] = true;
        }
      }
    }

    for(auto& elem : copyDecorators) {
      elem->preReadAndHandleExceptions(TransferType::read);
      if(elem->_activeException) {
        for(auto& lowLevelElem : elem->getHardwareAccessingElements()) {
          lowLevelElementsAndExceptionFlags[lowLevelElem] = true;
        }
      }
    }

    for(auto& it : lowLevelElementsAndExceptionFlags) {
      auto& elem = it.first;
      auto& hasSeenException = it.second;
      if(!hasSeenException) {
        elem->handleTransferException([&] { elem->readTransfer(); });
      }
    }

    auto exceptionHandlingResult = runPostReads(copyDecorators);
    exceptionHandlingResult += runPostReads(highLevelElements);

    exceptionHandlingResult.reThrow();
  }

  /*********************************************************************************************************************/

  void TransferGroup::write(VersionNumber versionNumber) {
    if(isReadOnly()) {
      throw ChimeraTK::logic_error("TransferGroup::write() called, but the TransferGroup is read-only.");
    }
    for(auto& it : lowLevelElementsAndExceptionFlags) {
      it.second = false;
    }

    for(auto& elem : highLevelElements) {
      elem->preWriteAndHandleExceptions(TransferType::write, versionNumber);
      if(elem->_activeException) {
        for(auto& lowLevelElem : elem->getHardwareAccessingElements()) {
          lowLevelElementsAndExceptionFlags[lowLevelElem] = true;
        }
      }
    }

    for(auto& it : lowLevelElementsAndExceptionFlags) {
      auto& elem = it.first;
      auto& hasSeenException = it.second;
      if(!hasSeenException) {
        elem->handleTransferException([&] { elem->writeTransfer(versionNumber); });
      }
    }

    ExceptionHandlingResult exceptionHandlingResult;
    for(auto& elem : highLevelElements) {
      exceptionHandlingResult = handlePostExceptions([&] { elem->postWrite(TransferType::write, versionNumber); });
    }

    exceptionHandlingResult.reThrow();
  }

  /*********************************************************************************************************************/

  bool TransferGroup::isReadOnly() { return readOnly; }

  /*********************************************************************************************************************/

  void TransferGroup::addAccessor(TransferElementAbstractor& accessor) {
    // check if accessor is already in a transfer group
    if(accessor.getHighLevelImplElement()->_isInTransferGroup) {
      throw ChimeraTK::logic_error("The given accessor is already in a TransferGroup and cannot be added "
                                   "to another.");
    }

    // set flag on the accessors that it is now in a transfer group
    accessor.getHighLevelImplElement()->_isInTransferGroup = true;

    auto highLevelElementsWithNewAccessor = highLevelElements;
    highLevelElementsWithNewAccessor.insert(accessor.getHighLevelImplElement());

    // try replacing all internal elements in all high-level elements
    for(auto& hlElem1 : highLevelElementsWithNewAccessor) {
      auto list = hlElem1->getInternalElements();
      list.push_front(hlElem1);

      for(auto& replacement : list) {
        // try on the abstractor first, to make sure we replace at the highest
        // level if possible
        accessor.replaceTransferElement(replacement);
        // try on all high-level elements already stored in the list
        for(auto& hlElem : highLevelElementsWithNewAccessor) {
          hlElem->replaceTransferElement(replacement); // note: this does nothing, if the replacement cannot
                                                       // be used by the hlElem!
        }
      }
    }

    // store the accessor in the list of high-level elements
    // this must be done only now, since it may have been replaced during the
    // replacement process
    highLevelElements.insert(accessor.getHighLevelImplElement());

    // update the list of hardware-accessing elements, since we might just have
    // made some of them redundant since we are using a set to store the elements,
    // duplicates are intrinsically avoided.
    lowLevelElementsAndExceptionFlags.clear();
    for(auto& hlElem : highLevelElements) {
      for(auto& hwElem : hlElem->getHardwareAccessingElements())
        lowLevelElementsAndExceptionFlags.insert({hwElem, false});
    }

    // update the list of CopyRegisterDecorators
    copyDecorators.clear();
    for(auto& hlElem : highLevelElements) {
      if(boost::dynamic_pointer_cast<ChimeraTK::CopyRegisterDecoratorTrait>(hlElem) != nullptr) {
        copyDecorators.insert(hlElem);
      }
      for(auto& hwElem : hlElem->getInternalElements()) {
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
    /// just used in TransferGroup::addAccessor(const
    /// boost::shared_ptr<TransferElement> &accessor)
    struct TransferGroupTransferElementAbstractor : TransferElementAbstractor {
      TransferGroupTransferElementAbstractor(boost::shared_ptr<TransferElement> impl)
      : TransferElementAbstractor(impl) {}

      void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) {
        _impl->replaceTransferElement(newElement);
      }
    };
  } // namespace detail

  /*********************************************************************************************************************/

  void TransferGroup::addAccessor(const boost::shared_ptr<TransferElement>& accessor) {
    /// @todo implement smarter and more efficient!
    auto x = detail::TransferGroupTransferElementAbstractor(accessor);
    addAccessor(x);
  }

  /*********************************************************************************************************************/

  void TransferGroup::dump() {
    std::cout << "=== Accessors added to this group: " << std::endl;
    for(auto& elem : highLevelElements) {
      std::cout << " - " << elem->getName() << std::endl;
    }
    std::cout << "=== Low-level transfer elements in this group: " << std::endl;
    for(auto& elem : lowLevelElementsAndExceptionFlags) {
      std::cout << " - " << elem.first->getName() << std::endl;
    }
    std::cout << "===" << std::endl;
  }

} /* namespace ChimeraTK */
