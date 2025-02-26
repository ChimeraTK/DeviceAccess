// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "DataConsistencyDecorator.h"

#include "ReadAnyGroup.h"

#include <boost/pointer_cast.hpp>

namespace ChimeraTK {

  template<typename UserType>
  DataConsistencyDecorator<UserType>::DataConsistencyDecorator(
      const boost::shared_ptr<NDRegisterAccessor<UserType>>& target, HDataConsistencyGroup* dGroup)
  : NDRegisterAccessorDecorator<UserType, UserType>(target), _dGroup(dGroup) {
    // check TransferElement is in ReadAnyGroup, direct reads do not make sense with HDataConsistencyGroup
    if(!target->getReadAnyGroup()) {
      throw logic_error(
          "Attempt to use DataConsistencyDecorator on TransferElement not in ReadAnyGroup: " + target->getName());
    }
    this->_inReadAnyGroup = target->getReadAnyGroup();

    this->_readQueue = target->getReadQueue().template then<void>([this]() { readCallback(); }, std::launch::deferred);
  }

  template<typename UserType>
  void DataConsistencyDecorator<UserType>::doPostRead(TransferType type, bool updateDataBuffer) {
    // we overwrite implementation of base class NDRegisterAccessorDecorator because we
    // update target user buffer already in readCallback, and then must not call target->postRead again here,
    // unless an exception originates from target->queue or its continuation(readCallback).
    if(this->_activeException) {
      _target->setActiveException(this->_activeException);
      _target->postRead(type, updateDataBuffer);
    }

    // Decorators have to copy meta data even if updateDataBuffer is false
    auto transferElementId = this->getId();
    auto& pe = this->_dGroup->getPushElements().at(transferElementId);

    if(pe.lastMatchingIndex > 0) {
      this->_dataValidity = pe.dataValidities[pe.lastMatchingIndex - 1];
      this->_versionNumber = pe.versionNumbers[pe.lastMatchingIndex - 1];
    }
    else {
      this->_dataValidity = _target->dataValidity();
      this->_versionNumber = _target->getVersionNumber();
    }

    if(!updateDataBuffer) {
      return;
    }
    if(pe.lastMatchingIndex > 0) {
      using UserBufferType = std::vector<std::vector<UserType>>;
      auto& bufferVector = *(this->_dGroup->template getBufferVector<UserBufferType>(transferElementId));
      auto& matchingBuffer = bufferVector[pe.lastMatchingIndex - 1];

      for(size_t i = 0; i < _target->getNumberOfChannels(); ++i) {
        buffer_2D[i].swap(matchingBuffer[i]);
      }
    }
    else {
      for(size_t i = 0; i < _target->getNumberOfChannels(); ++i) {
        buffer_2D[i].swap(_target->accessChannel(i));
      }
    }
  }

  template<typename UserType>
  void DataConsistencyDecorator<UserType>::readCallback() {
    // While ReadAnyGroup.waitAny() already calls preRead, waitAnyNonBlocking() does not,
    // but when it peeks into the readQueue, readCallback() is allready exectued!
    // We call target->preRead just to be sure; if called twice no harm done.
    // TODO discuss - Using TransferType::read is fine even in non-block case, since
    // readCallback is only executed when we know something is in target's readQueue.
    this->preRead(TransferType::read);
    // TODO discuss - preRead also needs to be called for decorator! is it fine to do it from here?

    // Before we update target buffer, we swap it into history. If history buffers = {h1, h2}, then switch h1<->h2,
    // target<->h1, so target oldest data (h2) which may be overwritten next. In match search (after
    // postRead), the three buffers {target, h1, h2} should be searched.
    _dGroup->updateHistory(_target->getId());

    // Differently from usual decorator behavior, we call target->postRead already here,
    // because we need user buffer content to judge data consistency.
    _target->postRead(TransferType::read, true);
    std::cout << "readCallback: seeing update for target " << _target->getName() << " vs "
              << _target->getVersionNumber() << std::endl;
    // Since we do not hand out all updates, we cannot rely on readAny() calling our preRead after
    // postRead. So we explicitly call preRead here, for the target.
    _target->preRead(TransferType::read);

    // check data consistency, including history, and update user buffers if necessary
    _dGroup->update(_target->getId());
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK

INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(ChimeraTK::DataConsistencyDecorator);
