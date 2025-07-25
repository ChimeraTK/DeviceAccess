// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "TransferGroup.h"

#include "CopyRegisterDecorator.h"
#include "Exception.h"
#include "TransferElement.h"
#include "TransferElementAbstractor.h"

#include <iostream>

namespace ChimeraTK {

  /********************************************************************************************************************/

  void TransferGroup::runPostReads(const std::set<boost::shared_ptr<TransferElement>>& elements,
      const std::exception_ptr& firstDetectedRuntimeError) {
    for(const auto& elem : elements) {
      // check for exceptions on any of the element's low level elements
      for(const auto& lowLevelElem : elem->getHardwareAccessingElements()) {
        // In case there are multiple exceptions we take the last one, but this does not matter. They are all
        // ChimeraTK::runtime_errors and the first detected runtime error, which is re-thrown, has already been
        // determined by previously.
        if(lowLevelElem->_activeException) {
          // copy the runtime error from low level element into the high level element so it is processed in  post-read
          elem->_activeException = lowLevelElem->_activeException;
        }
      }

      try {
        // call with updateDataBuffer = false if there has been any ChimeraTK::runtime_error in the transfer phase,
        // true otherwise
        elem->postReadAndHandleExceptions(TransferType::read, firstDetectedRuntimeError == nullptr);
      }
      catch(ChimeraTK::runtime_error&) {
        ++_nRuntimeErrors;
      }
    }
  }

  /********************************************************************************************************************/

  void TransferGroup::read() {
    // reset exception flags
    for(auto& it : _lowLevelElementsAndExceptionFlags) {
      it.second = false;
    }

    std::exception_ptr firstDetectedRuntimeError{nullptr};

    for(const auto& elem : _highLevelElements) {
      elem->preReadAndHandleExceptions(TransferType::read);
      if((elem->_activeException != nullptr) && (firstDetectedRuntimeError == nullptr)) {
        firstDetectedRuntimeError = elem->_activeException;
      }
    }

    // Call preRead on the copy decorators so that we can call postRead later with
    // readTransactionInProgress flag set to true and do the actual copying in the
    // decorator's doPostRead
    for(const auto& elem : _copyDecorators) {
      elem->preReadAndHandleExceptions(TransferType::read);
      if((elem->_activeException != nullptr) && (firstDetectedRuntimeError == nullptr)) {
        firstDetectedRuntimeError = elem->_activeException;
      }
    }

    if(firstDetectedRuntimeError == nullptr) {
      // only execute the transfers if there has been no exception yet
      for(const auto& it : _lowLevelElementsAndExceptionFlags) {
        const auto& elem = it.first;
        elem->handleTransferException([&] { elem->readTransfer(); });
        if((elem->_activeException != nullptr) && (firstDetectedRuntimeError == nullptr)) {
          firstDetectedRuntimeError = elem->_activeException;
        }
      }
    }

    // Exceptions from copy decorators are ignored. The same exception will be thrown by their target accessors in the
    // second call to runPostReads() anyway. In case the target is decorated with an ExceptionHandlingDecorator (in
    // ApplicationCore), the copy decorators exception would not be handled by an ExceptionHandlingDecorator and hence
    // terminate the application.
    _nRuntimeErrors = 0;
    runPostReads(_copyDecorators, firstDetectedRuntimeError);
    _nRuntimeErrors = 0;
    runPostReads(_highLevelElements, firstDetectedRuntimeError);

    // re-throw exceptions in the order of occurrence

    // The runtime error which was seen as _activeException in the pre or transfer phase might
    // have been handled in postRead, and thus become invalid (for instance because it
    // is suppressed by the ApplicationCore::ExceptionHandlingDecorator).
    // Only re-throw here if an exception has been re-thrown in the postRead step.
    // FIXME(?): If the firstDetectedRuntimeError has been handled, but another runtime_error
    // has been thrown, this logic will throw the wrong exception here. It could be
    // prevented with a complicated logic that loops all lowLevelElements in the order
    // that was used to execute the transfer, for each of them it loops all its associated high
    // level element, and for each high level element it loops all the low level elements again
    // because some might and others might not have exceptions, and if there was an exception for
    // that high level element on any of its low level elements, postRead must be called with
    // an exception set each time. (I guess this sentence in close to not understandable, that's
    // why I am not trying to implement it).
    // In practice this will not happen because either all elements will have an ExceptionHandlingDecorator, or none.

    if(_nRuntimeErrors != 0) {
      assert(firstDetectedRuntimeError != nullptr); // postRead must only rethrow, so there must be a detected
                                                    // runtime_error
      _cachedReadableWriteableIsValid = false;
      std::rethrow_exception(firstDetectedRuntimeError);
    }
  } // namespace ChimeraTK

  /********************************************************************************************************************/

  void TransferGroup::write(VersionNumber versionNumber) {
    for(auto& it : _lowLevelElementsAndExceptionFlags) {
      it.second = false;
    }

    std::exception_ptr firstDetectedRuntimeError{nullptr};
    for(const auto& elem : _highLevelElements) {
      elem->preWriteAndHandleExceptions(TransferType::write, versionNumber);
      if((elem->_activeException != nullptr) && (firstDetectedRuntimeError == nullptr)) {
        firstDetectedRuntimeError = elem->_activeException;
      }
    }

    if(firstDetectedRuntimeError == nullptr) {
      for(const auto& it : _lowLevelElementsAndExceptionFlags) {
        const auto& elem = it.first;
        elem->handleTransferException([&] { elem->writeTransfer(versionNumber); });
        if((elem->_activeException != nullptr) && (firstDetectedRuntimeError == nullptr)) {
          firstDetectedRuntimeError = elem->_activeException;
        }
      }
    }

    _nRuntimeErrors = 0;
    for(const auto& elem : _highLevelElements) {
      // check for exceptions on any of the element's low level elements
      for(const auto& lowLevelElem : elem->getHardwareAccessingElements()) {
        // In case there are multiple exceptions we take the last one, but this does not matter. They are all
        // ChimeraTK::runtime_errors and the first detected runtime error, which is re-thrown, has already been
        // determined by previously.
        if(lowLevelElem->_activeException) {
          // copy the runtime error from low level element into the high level element so it is processed in  post-read
          elem->_activeException = lowLevelElem->_activeException;
        }
      }

      try {
        elem->postWrite(TransferType::write, versionNumber);
      }
      catch(ChimeraTK::runtime_error&) {
        ++_nRuntimeErrors;
      }
    }

    if(_nRuntimeErrors != 0) {
      assert(firstDetectedRuntimeError != nullptr);
      _cachedReadableWriteableIsValid = false;
      std::rethrow_exception(firstDetectedRuntimeError);
    }
  }

  /********************************************************************************************************************/

  bool TransferGroup::isReadOnly() {
    return isReadable() && !isWriteable();
  }

  /********************************************************************************************************************/

  bool TransferGroup::isReadable() {
    if(!_cachedReadableWriteableIsValid) {
      updateIsReadableWriteable();
    }
    return _isReadable;
  }

  /********************************************************************************************************************/

  bool TransferGroup::isWriteable() {
    if(!_cachedReadableWriteableIsValid) {
      updateIsReadableWriteable();
    }
    return _isWriteable;
  }

  /********************************************************************************************************************/

  void TransferGroup::addAccessorImpl(TransferElementAbstractor& accessor, bool isTemporary) {
    // check if accessor is already in a transfer group
    if(accessor.getHighLevelImplElement()->_isInTransferGroup) {
      throw ChimeraTK::logic_error("The given accessor is already in a TransferGroup and cannot be added "
                                   "to another.");
    }

    // Only accessors without wait_for_new_data can be used in a transfer group.
    if(accessor.getAccessModeFlags().has(AccessMode::wait_for_new_data)) {
      throw ChimeraTK::logic_error(
          "A TransferGroup can only be used with transfer elements that don't have aAccessMode::wait_for_new_data.");
    }

    // set flag on the accessors that it is now in a transfer group
    accessor.getHighLevelImplElement()->_isInTransferGroup = true;

    auto highLevelElementsWithNewAccessor = _highLevelElements;
    highLevelElementsWithNewAccessor.insert(accessor.getHighLevelImplElement());

    // try replacing all internal elements in all high-level elements
    for(const auto& hlElem1 : highLevelElementsWithNewAccessor) {
      auto list = hlElem1->getInternalElements();
      list.push_front(hlElem1);

      for(const auto& replacement : list) {
        // try on the abstractor first, to make sure we replace at the highest
        // level if possible, but only if this isn't a temporary abstractor
        if(not isTemporary) accessor.replaceTransferElement(replacement);
        // try on all high-level elements already stored in the list
        for(const auto& hlElem : highLevelElementsWithNewAccessor) {
          hlElem->replaceTransferElement(replacement); // note: this does nothing, if the replacement cannot
                                                       // be used by the hlElem!
        }
      }
    }

    // store the accessor in the list of high-level elements
    // this must be done only now, since it may have been replaced during the
    // replacement process
    _highLevelElements.insert(accessor.getHighLevelImplElement());

    // update the list of hardware-accessing elements, since we might just have
    // made some of them redundant since we are using a set to store the elements,
    // duplicates are intrinsically avoided.
    _lowLevelElementsAndExceptionFlags.clear();
    for(const auto& hlElem : _highLevelElements) {
      for(const auto& hwElem : hlElem->getHardwareAccessingElements()) {
        _lowLevelElementsAndExceptionFlags.insert({hwElem, false});
      }
    }

    // update the list of CopyRegisterDecorators
    _copyDecorators.clear();
    for(const auto& hlElem : _highLevelElements) {
      if(boost::dynamic_pointer_cast<ChimeraTK::CopyRegisterDecoratorTrait>(hlElem) != nullptr) {
        _copyDecorators.insert(hlElem);
      }
      for(const auto& hwElem : hlElem->getInternalElements()) {
        if(boost::dynamic_pointer_cast<ChimeraTK::CopyRegisterDecoratorTrait>(hwElem) != nullptr) {
          _copyDecorators.insert(hwElem);
        }
      }
    }

    // read-write flag may need to be updated
    _cachedReadableWriteableIsValid = false;
  }

  /********************************************************************************************************************/

  namespace detail {
    /// just used in TransferGroup::addAccessor(const
    /// boost::shared_ptr<TransferElement> &accessor)
    struct TransferGroupTransferElementAbstractor : TransferElementAbstractor {
      explicit TransferGroupTransferElementAbstractor(boost::shared_ptr<TransferElement> impl)
      : TransferElementAbstractor(std::move(impl)) {}

      void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) {
        _impl->replaceTransferElement(std::move(newElement));
      }
    };
  } // namespace detail

  /********************************************************************************************************************/

  void TransferGroup::addAccessor(const boost::shared_ptr<TransferElement>& accessor) {
    /// @todo implement smarter and more efficient!
    auto x = detail::TransferGroupTransferElementAbstractor(accessor);
    addAccessorImpl(x, true);
  }

  /********************************************************************************************************************/

  void TransferGroup::addAccessor(TransferElementAbstractor& accessor) {
    addAccessorImpl(accessor, false);
  }

  /********************************************************************************************************************/

  void TransferGroup::updateIsReadableWriteable() {
    _isReadable = true;
    _isWriteable = true;
    for(const auto& elem : _highLevelElements) {
      if(!elem->isReadable()) _isReadable = false;
      if(!elem->isWriteable()) _isWriteable = false;
    }
    _cachedReadableWriteableIsValid = true;
  }

  /********************************************************************************************************************/

  void TransferGroup::dump() {
    std::cout << "=== Accessors added to this group: " << std::endl;
    for(const auto& elem : _highLevelElements) {
      std::cout << " - " << elem->getName() << std::endl;
    }
    std::cout << "=== Low-level transfer elements in this group: " << std::endl;
    for(auto& elem : _lowLevelElementsAndExceptionFlags) {
      std::cout << " - " << elem.first->getName() << std::endl;
    }
    std::cout << "===" << std::endl;
  }

  /********************************************************************************************************************/

} /* namespace ChimeraTK */
