// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "DataConsistencyGroupHistorizedMatcher.h"
#include "NDRegisterAccessorDecorator.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  /**
   * A quasi-decorator to be used to decorate targets which should provide data updates only
   * when data is consistent (i.e. data with matching VersionNumber is available).
   * In that case, all decorated targets are updated, via postRead which updates their user buffers.
   * DataConsistencyDecorator internally historizes data updates including meta data, and looks for matches in the
   * history. It shall be used together with a ReadAnyGroup containing at least all decorated elements.
   */
  template<typename UserType>
  class DataConsistencyDecorator : public NDRegisterAccessorDecorator<UserType, UserType> {
   public:
    DataConsistencyDecorator(const boost::shared_ptr<NDRegisterAccessor<UserType>>& target,
        DataConsistencyGroupDetail::HistorizedMatcher* dGroup);

    void doPreRead(TransferType type) override;

    void doPostRead(TransferType type, bool updateDataBuffer) override;
    // continuation / callback for future_queue
    void readCallback();

   protected:
    DataConsistencyGroupDetail::HistorizedMatcher* _hGroup;

    using NDRegisterAccessorDecorator<UserType>::_target;
    using NDRegisterAccessorDecorator<UserType>::buffer_2D;
  };

  /********************************************************************************************************************/

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(DataConsistencyDecorator);

  /********************************************************************************************************************/

} /* namespace ChimeraTK */
