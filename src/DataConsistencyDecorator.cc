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
    }

    this->_readQueue = target->getReadQueue().template then<void>([this]() { readCallback(); }, std::launch::deferred);
  }

  template<typename T>
  void DataConsistencyDecorator<T>::doPostRead(TransferType type, bool updateDataBuffer) {
    // TODO check - do we need own implementation?
    // problem is, we want target buffer update earlier!

    // TODO discuss exception handling - I'm currently pretty lost
    _target->setActiveException(this->_activeException);
    // we leave out target->postRead since it was done earlier
    //_target->postRead(type, updateDataBuffer);

    // Decorators have to copy meta data even if updateDataBuffer is false
    this->_dataValidity = _target->dataValidity();
    this->_versionNumber = _target->getVersionNumber();

    if(!updateDataBuffer) return;
    for(size_t i = 0; i < _target->getNumberOfChannels(); ++i) {
      buffer_2D[i].swap(_target->accessChannel(i));
    }
  }

  template<typename T>
  void DataConsistencyDecorator<T>::readCallback() {
    // TODO check -Probably will not be called on exception in target
    // so we should test a situation where target throws an exception

    // we know that target->preRead was already called.
    // Differently from usual decorator behavior, we call target->postRead already here,
    // because we need user buffer content to judge data consistency.
    _target->postRead(TransferType::read, true);

    // check data consistency, including history, and update user buffers if necessary
    _dGroup->update(_target->getId());
  }

  /********************************************************************************************************************/

  HDataConsistencyGroup::HDataConsistencyGroup(
      std::initializer_list<std::reference_wrapper<TransferElementAbstractor>> list) {
    for(const auto& element : list) add(element);
    decorateAccessors();
  }

  void HDataConsistencyGroup::decorateAccessors() {
    // TODO decoration magic: we need a function that replaces target of
    // TransferElementAbstractor by DataConsistencyDecorator(target)
    // replacement must happen below (i.e. for target of) MetaDataPropagatingRegisterDecorator

    std::map<TransferElementID, TransferElementAbstractor>& elements = getElements();

    for(auto& e : elements) {
      auto& acc = e.second;
      callForType(acc.getValueType(), [&](auto t) {
        using UserType = decltype(t);

        // let's not replace if the target is already a DataConsistencyDecorator
        auto alreadyDecorated =
            boost::dynamic_pointer_cast<DataConsistencyDecorator<UserType>>(acc.getHighLevelImplElement());
        if(!alreadyDecorated) {
          // TODO discuss/check - is it clear that cast to NDRegisterAccessor will work?
          auto accImpl = boost::dynamic_pointer_cast<NDRegisterAccessor<UserType>>(acc.getHighLevelImplElement());
          assert(accImpl);
          acc.replace(boost::make_shared<DataConsistencyDecorator<UserType>>(accImpl, this));
        }
      });
    }
  }
  void HDataConsistencyGroup::decorateAccessors(ReadAnyGroup* rag) {
    std::map<TransferElementID, TransferElementAbstractor>& elements = getElements();
    // replace accessors stored in ReadAnyGroup by our decorated versions, if applicable
    for(auto& acc : rag->push_elements) {
      // id of the target accessor = id of decorator
      auto id = acc.getId();
      if(elements.find(id) != elements.end()) {
        acc.replace(elements.at(id));
      }
    }
  }

  bool HDataConsistencyGroup::update(const TransferElementID& transferElementID) {
    bool consistent = DataConsistencyGroup::update(transferElementID);

    if(!consistent) {
      // if not consistent, delay call to postRead (called from ReadAnyGroup), by throwing DiscardValueException
      // add to set of decorators needing call to postRead later
      decoratorsNeedingPostRead.insert(transferElementID);
      throw detail::DiscardValueException();
    }

    // to update other user buffers, call postRead on the other involved decorators
    for(TransferElementID id : decoratorsNeedingPostRead) {
      auto acc = getElements().at(id);
      // TODO here we should check for exceptions:
      // it is allowed for postRead to throw exceptions; what would be our expectation then?
      // shoule we remove single items from decoratorsNeedingPostRead ?
      acc.getHighLevelImplElement()->postRead(TransferType::read, true);
    }
    // our own postRead will be called by ReadAnyGroup
    decoratorsNeedingPostRead.clear();
    return consistent;
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK

INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(ChimeraTK::DataConsistencyDecorator);
