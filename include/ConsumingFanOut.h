/*
 * ConsumingFanOut.h
 *
 *  Created on: Jun 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_CONSUMING_FAN_OUT_H
#define CHIMERATK_CONSUMING_FAN_OUT_H

#include <ChimeraTK/NDRegisterAccessorDecorator.h>

#include "FanOut.h"

namespace ChimeraTK {

  /** FanOut implementation which acts as a read-only (i.e. consuming)
   * NDRegisterAccessor. The values read through this accessor will be obtained
   * from the given feeding implementation and distributed to any number of
   * slaves. */
  template<typename UserType>
  class ConsumingFanOut : public FanOut<UserType>, public ChimeraTK::NDRegisterAccessorDecorator<UserType> {
   public:
    ConsumingFanOut(boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> feedingImpl)
    : FanOut<UserType>(feedingImpl), ChimeraTK::NDRegisterAccessorDecorator<UserType>(feedingImpl) {
      assert(feedingImpl->isReadable());
    }

    void doPostRead(TransferType type, bool hasNewData) override {
      ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPostRead(type, hasNewData);
      for(auto& slave : FanOut<UserType>::slaves) { // send out copies to slaves
        // do not send copy if no data is expected (e.g. trigger)
        if(slave->getNumberOfSamples() != 0) {
          slave->accessChannel(0) = buffer_2D[0];
        }
        slave->setDataValidity(this->dataValidity());
        slave->writeDestructively();
      }
    }

    void interrupt() override {
      // call the interrut sequences of the fan out (interrupts for fan input and all outputs), and the ndRegisterAccessor
      FanOut<UserType>::interrupt();
      ChimeraTK::NDRegisterAccessor<UserType>::interrupt();
    }

   protected:
    using ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_CONSUMING_FAN_OUT_H */
