// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "DataConsistencyGroup.h"
#include "NDRegisterAccessorDecorator.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  class ReadAnyGroup;

  class HDataConsistencyGroup : public DataConsistencyGroup {
   public:
    /**
     * Construct HDataConsistencyGroup from a set of TransferElementAbstractors. These will be decorated
     * with DataConsistencyDecorator.
     * Note, constructor for set of TransferElements is not provided since we could not decorate them for the caller.
     */
    HDataConsistencyGroup(std::initializer_list<std::reference_wrapper<TransferElementAbstractor>> list);

    /// decorate accessor by replacing its target => DataConsistencyDecorator(target)
    void decorateAccessor(TransferElementAbstractor& acc);
    /// use known already decorated accessors as replacement for ReadAnyGroup's content
    void decorateAccessors(ReadAnyGroup* rag);

    /**
     * Note, differently from DataConsistencyGroup, explicit call to this function is no longer required,
     * since already done by DataConsistencyDecorator.
     */
    bool update(const TransferElementID& transferElementID);

   protected:
    // user may no longer directly call add from base class
    using DataConsistencyGroup::add;

    std::set<TransferElementID> decoratorsNeedingPostRead;
    std::map<TransferElementID, TransferElementAbstractor> decoratedElements;
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
    DataConsistencyDecorator(const boost::shared_ptr<NDRegisterAccessor<T>>& target, HDataConsistencyGroup* dGroup);

    void doPostRead(TransferType type, bool updateDataBuffer) override;
    // continuation / callback for future_queue
    void readCallback();

   protected:
    HDataConsistencyGroup* _dGroup;

    using NDRegisterAccessorDecorator<T>::_target;
    using NDRegisterAccessorDecorator<T>::buffer_2D;
  };

  /********************************************************************************************************************/

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(DataConsistencyDecorator);

  /********************************************************************************************************************/

} /* namespace ChimeraTK */
