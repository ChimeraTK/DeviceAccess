// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "DataConsistencyDecorator.h"

#include "ReadAnyGroup.h"

#include <boost/pointer_cast.hpp>

namespace ChimeraTK {

  template<typename T>
  DataConsistencyDecorator<T>::DataConsistencyDecorator(
      const boost::shared_ptr<NDRegisterAccessor<T>>& target, HDataConsistencyGroup* dGroup)
  : NDRegisterAccessorDecorator<T, T>(target), _dGroup(dGroup) {
    // check TransferElement is in ReadAnyGroup, direct reads do not make sense with HDataConsistencyGroup
    if(!target->isInReadAnyGroup()) {
      throw logic_error(
          "Attempt to use DataConsistencyDecorator on TransferElement not in ReadAnyGroup: " + target->getName());
      this->_isInReadAnyGroup = true;
    }

    this->_readQueue = target->getReadQueue().template then<void>([this]() { readCallback(); }, std::launch::deferred);
  }

  template<typename T>
  void DataConsistencyDecorator<T>::doPostRead(TransferType type, bool updateDataBuffer) {
    // we overwrite implementation of base class NDRegisterAccessorDecorator because we
    // update target user buffer already in readCallback, and then must not call target->postRead again here,
    // unless an exception originates from target->queue or its continuation(readCallback).
    if(this->_activeException) {
      _target->setActiveException(this->_activeException);
      _target->postRead(type, updateDataBuffer);
    }

    // Decorators have to copy meta data even if updateDataBuffer is false
    this->_dataValidity = _target->dataValidity();
    this->_versionNumber = _target->getVersionNumber();

    if(!updateDataBuffer) {
      return;
    }
    for(size_t i = 0; i < _target->getNumberOfChannels(); ++i) {
      buffer_2D[i].swap(_target->accessChannel(i));
    }
  }

  template<typename T>
  void DataConsistencyDecorator<T>::readCallback() {
    // While ReadAnyGroup.waitAny() already calls preRead, waitAnyNonBlocking() does not,
    // but when it peeks into the readQueue, readCallback() is allready exectued!
    // We call target->preRead just to be sure; if called twice no harm done.
    // TODO discuss - Using TransferType::read is fine even in non-block case, since
    // readCallback is only executed when we know something is in target's readQueue.
    this->preRead(TransferType::read);
    // TODO discuss - preRead also needs to be called for decorator! is it fine to do it from here?

    // Differently from usual decorator behavior, we call target->postRead already here,
    // because we need user buffer content to judge data consistency.
    _target->postRead(TransferType::read, true);

    std::cout << "readCallback: seeing update for target " << _target->getName() << " vs "
              << _target->getVersionNumber() << std::endl;

    // check data consistency, including history, and update user buffers if necessary
    _dGroup->update(_target->getId());
  }

  /********************************************************************************************************************/

  HDataConsistencyGroup::HDataConsistencyGroup(
      std::initializer_list<std::reference_wrapper<TransferElementAbstractor>> list) {
    for(TransferElementAbstractor& acc : list) {
      // add target accessor to DataConsistencyGroup
      add(acc);
      decorateAccessor(acc);
      // add decorated access to our elements map (key = Id remains unchanged by decoration)
      decoratedElements[acc.getId()] = acc;
    }
  }

  void HDataConsistencyGroup::decorateAccessor(TransferElementAbstractor& acc) {
    callForType(acc.getValueType(), [&](auto t) {
      using UserType = decltype(t);

      // let's not replace if the target is already a DataConsistencyDecorator
      auto alreadyDecorated =
          boost::dynamic_pointer_cast<DataConsistencyDecorator<UserType>>(acc.getHighLevelImplElement());
      if(!alreadyDecorated) {
        auto accImpl = boost::dynamic_pointer_cast<NDRegisterAccessor<UserType>>(acc.getHighLevelImplElement());
        assert(accImpl);
        acc.replace(boost::make_shared<DataConsistencyDecorator<UserType>>(accImpl, this));
      }
    });
    // TODO decoration magic for MetaDataPropagatingRegisterDecorator:
    // replacement must happen below (i.e. for target of) MetaDataPropagatingRegisterDecorator
  }

  void HDataConsistencyGroup::decorateAccessors(ReadAnyGroup* rag) {
    // replace accessors stored in ReadAnyGroup by our decorated versions, if applicable
    for(auto& acc : rag->push_elements) {
      // id of the target accessor = id of decorator
      auto id = acc.getId();
      if(decoratedElements.find(id) != decoratedElements.end()) {
        acc.replace(decoratedElements.at(id));
      }
    }
  }

  bool HDataConsistencyGroup::update(const TransferElementID& transferElementID) {
    bool consistent = DataConsistencyGroup::update(transferElementID);

    if(!consistent) {
      // if not consistent, delay call to postRead (called from ReadAnyGroup), by throwing DiscardValueException.
      // Then, ReadAnyGroup leaves out postRead and next preRead.
      // Add to set of decorators needing call to postRead later.
      decoratorsNeedingPostRead.insert(transferElementID);
      throw detail::DiscardValueException();
    }

    // to update other user buffers, call postRead on the other involved decorators
    // Note, in case of an exception thrown by some postRead, it might happen that postRead is
    // called more than once in a row, for the other elements. This is allowed.
    for(TransferElementID id : decoratorsNeedingPostRead) {
      auto& acc = decoratedElements.at(id);
      acc.getHighLevelImplElement()->postRead(TransferType::read, true);
      // just as in ReadAnyGroup, we immediately follow-up with preRead
      acc.getHighLevelImplElement()->preRead(TransferType::read);
    }
    // our own postRead will be called by ReadAnyGroup
    decoratorsNeedingPostRead.clear();
    return consistent;
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK

INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(ChimeraTK::DataConsistencyDecorator);
