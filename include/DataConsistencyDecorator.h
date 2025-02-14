// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "DataConsistencyGroup.h"
#include "NDRegisterAccessorDecorator.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  class HDataConsistencyGroup : public DataConsistencyGroup {
   public:
    HDataConsistencyGroup(std::initializer_list<TransferElementAbstractor> list);
    // TODO check - would we also need constructor from TransferElements?
    // to me it seems not, since then cannot decorate them for the outside.

    // TODO also add function? Maybe not, comes from base class

    void replacementMagic();

    bool update(const TransferElementID& transferElementID);

    std::set<TransferElementID> decoratorsNeedingPostRead;
  };

  /********************************************************************************************************************/

  /**
   * A quasi-decorator to be used to decorate targets which should provide data updates only
   * when data is consistent (i.e. data with matching VersionNumber is available).
   * In that case, all decorated targets are updated, via postRead which updates their user buffers.
   * DataConsistencyDecorator internally historizes data updates including meta data, and looks for matches in the
   * history. It shall be used together with a ReadAnyGroup containing at least all decorated elements.
   */
  template<typename T>
  class DataConsistencyDecorator : public NDRegisterAccessorDecorator<T, T> {
   public:
    DataConsistencyDecorator(const boost::shared_ptr<NDRegisterAccessor<T>>& target, HDataConsistencyGroup* dGroup)
    : NDRegisterAccessorDecorator<T, T>(target), _dGroup(dGroup) {
      // check TransferElement is in ReadAnyGroup, direct reads do not make sense with HDataConsistencyGroup
      if(!target->_isInReadAnyGroup) {
        throw logic_error(
            "Attempt to use DataConsistencyDecorator on TransferElement not in ReadAnyGroup: " + target->_name);
      }

      this->_readQueue = _target->_readQueue.template then<void>([this]() { readCallback(); }, std::launch::deferred);
    }

    void doPostRead(TransferType type, bool updateDataBuffer) override;
    // continuation / callback for future_queue
    void readCallback();

   protected:
    HDataConsistencyGroup* _dGroup;

    using TransferElement::_dataValidity;
    using NDRegisterAccessorDecorator<T>::_target;
    using NDRegisterAccessorDecorator<T>::buffer_2D;
  };

  /********************************************************************************************************************/

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(DataConsistencyDecorator);

  /********************************************************************************************************************/
  HDataConsistencyGroup::HDataConsistencyGroup(std::initializer_list<TransferElementAbstractor> list) {
    for(const auto& element : list) add(element);
    replacementMagic();
  }

  inline void HDataConsistencyGroup::replacementMagic() {
    // TODO decoration magic: we need a function that replaces target of
    // TransferElementAbstractor by DataConsistencyDecorator(target)
    // replacement must happen below (i.e. for target of) MetaDataPropagatingRegisterDecorator

    std::map<TransferElementID, TransferElementAbstractor> elements = getElements();

    for(auto e : elements) {
      auto acc = e.second;
      callForType(acc.getValueType(), [&](auto t) {
        using UserType = decltype(t);

        // let's not replace if the target is already a DataConsistencyDecorator
        auto alreadyDecorated =
            boost::dynamic_pointer_cast<DataConsistencyDecorator<UserType>>(acc.getHighLevelImplElement());
        if(!alreadyDecorated) {
          auto decorator = boost::make_shared<DataConsistencyDecorator<UserType>>(acc, this);
          acc.replace(decorator);
        }
      });
    }
  }

  inline bool HDataConsistencyGroup::update(const TransferElementID& transferElementID) {
    bool consistent = DataConsistencyGroup::update(transferElementID);

    if(!consistent) {
      // if not consistent, delay call to postRead (called from ReadAnyGroup), by throwing DiscardValueException
      // add to set of decorators needing call to postRead later
      decoratorsNeedingPostRead.insert(transferElementID);
      throw detail::DiscardValueException();
    }

    // TODO update other buffers, call postRead on the other involved decorators
    for(TransferElementID id : decoratorsNeedingPostRead) {
      auto acc = getElements().at(id);
      acc.getHighLevelImplElement()->postRead(TransferType::read, true);
    }
    // our own postRead will be called by ReadAnyGroup
    decoratorsNeedingPostRead.clear();
    return consistent;
  }

  /********************************************************************************************************************/

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
    // we know that target->preRead was already called.
    // Differently from usual decorator behavior, we call target->postRead already here,
    // because we need user buffer content to judge data consistency.
    _target->postRead(TransferType::read, true);

    // check data consistency, including history, and update user buffers if necessary
    _dGroup->update(_target->getId());
  }

  /********************************************************************************************************************/

} /* namespace ChimeraTK */
