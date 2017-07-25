/*
 * ConsumingFanOut.h
 *
 *  Created on: Jun 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_CONSUMING_FAN_OUT_H
#define CHIMERATK_CONSUMING_FAN_OUT_H

#include <mtca4u/NDRegisterAccessor.h>

#include "FanOut.h"

namespace ChimeraTK {

  /** FanOut implementation which acts as a read-only (i.e. consuming) NDRegisterAccessor. The values read through
   *  this accessor will be obtained from the given feeding implementation and distributed to any number of slaves. */
  template<typename UserType>
  class ConsumingFanOut : public FanOut<UserType>, public mtca4u::NDRegisterAccessor<UserType> {

    public:
      
      ConsumingFanOut(boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> feedingImpl)
      : FanOut<UserType>(feedingImpl)
      {
        mtca4u::NDRegisterAccessor<UserType>::buffer_2D.resize( feedingImpl->getNumberOfChannels() );
        for(size_t i=0; i<feedingImpl->getNumberOfChannels(); i++) {
          mtca4u::NDRegisterAccessor<UserType>::buffer_2D[i].resize( feedingImpl->getNumberOfSamples() );
        }
      }

      bool isReadable() const override {
        return true;
      }
      
      bool isReadOnly() const override {
        return true;
      }
      
      bool isWriteable() const override {
        return false;
      }
      
      void doReadTransfer() override {
        FanOut<UserType>::impl->read();
      }

      bool doReadTransferNonBlocking() override {
        return FanOut<UserType>::impl->readNonBlocking();
      }

      bool doReadTransferLatest() override {
        return FanOut<UserType>::impl->readLatest();
      }

      void postRead() override {
        mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0].swap(FanOut<UserType>::impl->accessChannel(0));
        for(auto &slave : FanOut<UserType>::slaves) {     // send out copies to slaves
          // do not send copy if no data is expected (e.g. trigger)
          if(slave->getNumberOfSamples() != 0) {
            slave->accessChannel(0) = mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0];
          }
          slave->write();
        }
      }

      bool write(ChimeraTK::VersionNumber /*versionNumber*/={}) override {
        throw std::logic_error("Write operation called on read-only variable.");
      }
      
      bool isSameRegister(const boost::shared_ptr<const mtca4u::TransferElement>& e) const override {
        // only true if the very instance of the transfer element is the same
        return e.get() == this;
      }
      
      std::vector<boost::shared_ptr<mtca4u::TransferElement> > getHardwareAccessingElements() override {
        return { boost::enable_shared_from_this<mtca4u::TransferElement>::shared_from_this() };
      }
      
      void replaceTransferElement(boost::shared_ptr<mtca4u::TransferElement>) override {
        // You can't replace anything here. Just do nothing.
      }

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_CONSUMING_FAN_OUT_H */

