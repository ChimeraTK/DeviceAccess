// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "DataConsistencyDecorator.h"

#include <boost/pointer_cast.hpp>

namespace ChimeraTK {

  template<typename UserType>
  DataConsistencyDecorator<UserType>::DataConsistencyDecorator(
      const boost::shared_ptr<NDRegisterAccessor<UserType>>& target,
      DataConsistencyGroupDetail::HistorizedMatcher* dGroup)
  : NDRegisterAccessorDecorator<UserType, UserType>(target), _hGroup(dGroup) {
    this->_inReadAnyGroup = target->getReadAnyGroup();
    this->_readQueue = target->getReadQueue().template then<void>([this]() { readCallback(); }, std::launch::deferred);
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void DataConsistencyDecorator<UserType>::doPreRead(TransferType type) {
    _hGroup->handleMissingPreReads(this->getId());
    _target->preRead(type);
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void DataConsistencyDecorator<UserType>::doPostRead(TransferType type, bool updateDataBuffer) {
    // In special case that we have consistent state with unchanged version number, we do not update other
    // decorator buffers.
    // E.g. assume that all decorators are in consistent state, say v3.
    // Now A gets a new update, v4. This is put into history, and since there is no value yet for matching B,
    // we throw DiscardValueException. Assume then, we get another value for B, with version v3.
    // In this case, decorator(A) must not be swapped again, but decorator(B) should be swapped with target(B)=v3
    if(_hGroup->lastMatchingVersionNumber() > this->_versionNumber) {
      _hGroup->handleMissingPostReads();
    }

    // we overwrite implementation of base class NDRegisterAccessorDecorator because we
    // update target user buffer already in readCallback, and then must not call target->postRead again here,
    // unless an exception originates from target->queue or its continuation(readCallback).
    if(this->_activeException) {
      _target->setActiveException(this->_activeException);
      _target->postRead(type, updateDataBuffer);
    }

    // Decorators have to copy meta data even if updateDataBuffer is false
    auto transferElementId = this->getId();
    this->_hGroup->getMatchingInfo(transferElementId, this->_versionNumber, this->_dataValidity);

    if(!updateDataBuffer) {
      return;
    }

    auto& matchingBuffer = this->_hGroup->template getMatchingBuffer<UserType>(transferElementId);
    for(size_t i = 0; i < _target->getNumberOfChannels(); ++i) {
      buffer_2D[i].swap(matchingBuffer[i]);
    }
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void DataConsistencyDecorator<UserType>::readCallback() {
    // While ReadAnyGroup.waitAny() already calls preRead at the beginning, waitAnyNonBlocking() does not,
    // and fist peeks into the readQueue, causing readCallback() to be executed first.
    // We call preRead just to be sure; if called twice no harm done.
    // Using TransferType::read should be fine even in non-blocking case, since
    // readCallback is only executed when we know something is in target's readQueue.
    _target->preRead(TransferType::read);

    // Before we update target buffer, we swap it into history. If history buffers = {h1, h2}, then switch h1<->h2,
    // target<->h1, so target oldest data (h2) which may be overwritten next. In match search (after
    // postRead), the three buffers {target, h1, h2} should be searched.
    _hGroup->updateHistory(_target->getId());

    // Differently from usual decorator behavior, we call target->postRead already here,
    // because we need user buffer content to judge data consistency.
    _target->postRead(TransferType::read, true);

    // check data consistency, including history, and update user buffers if necessary
    bool consistent = _hGroup->checkUpdate(_target->getId());
    if(!consistent) {
      // if not consistent, delay call to postRead (called from ReadAnyGroup), by throwing DiscardValueException.
      // Then, ReadAnyGroup leaves out postRead and next preRead.
      // In order to get another update for target, need to call preRead here.
      _target->preRead(TransferType::read);
      throw detail::DiscardValueException();
    }
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK

INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(ChimeraTK::DataConsistencyDecorator);
