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
    HDataConsistencyGroup(std::initializer_list<std::reference_wrapper<TransferElementAbstractor>> list);
    // TODO check - would we also need constructor from TransferElements?
    // to me it seems not, since then cannot decorate them for the outside.

    // TODO also add function? Maybe not, comes from base class

    void decorateAccessors();
    void decorateAccessors(ReadAnyGroup* rag);

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
