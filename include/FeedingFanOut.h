/*
 * FeedingFanOut.h
 *
 *  Created on: Jun 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_FEEDING_FAN_OUT_H
#define CHIMERATK_FEEDING_FAN_OUT_H

#include <mtca4u/NDRegisterAccessor.h>

#include "FanOut.h"

namespace ChimeraTK {

  /**
   * NDRegisterAccessor implementation which distributes values written to this accessor out to any number of slaves.
   */
  template<typename UserType>
  class FeedingFanOut : public FanOut<UserType>, public mtca4u::NDRegisterAccessor<UserType> {

    public:

      FeedingFanOut(std::string const &name, std::string const &unit, std::string const &description,
                    size_t numberOfElements)
      : FanOut<UserType>(boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>>()),
        mtca4u::NDRegisterAccessor<UserType>(name, unit, description)
      {
        try {
          mtca4u::NDRegisterAccessor<UserType>::buffer_2D.resize(1);
          mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0].resize(numberOfElements);
        }
        catch(...) {
          this->shutdown();
          throw;
        }
      }

      ~FeedingFanOut() {
        this->shutdown();
      }

      /** Add a slave to the FanOut. Only sending end-points of a consuming node may be added. */
      void addSlave(boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> slave) override {
        if(!slave->isWriteable()) {
          throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>(
              "FeedingFanOut::addSlave() has been called with a receiving implementation!");
        }
        // check if array shape is compatible, unless the receiver is a trigger node, so no data is expected
        if( slave->getNumberOfSamples() != 0 &&
            ( slave->getNumberOfChannels() != 1 || slave->getNumberOfSamples() != this->getNumberOfSamples() ) ) {
          std::string what = "FeedingFanOut::addSlave(): Trying to add a slave '" + slave->getName();
          what += "' with incompatible array shape! Name of fan out: '" + this->getName() + "'";
          throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>(what.c_str());
        }
        FanOut<UserType>::slaves.push_back(slave);
      }

      bool isReadable() const override {
        return false;
      }

      bool isReadOnly() const override {
        return false;
      }

      bool isWriteable() const override {
        return true;
      }

      void doReadTransfer() override {
        throw std::logic_error("Read operation called on write-only variable.");
      }

      bool doReadTransferNonBlocking() override {
        throw std::logic_error("Read operation called on write-only variable.");
      }

      bool doReadTransferLatest() override {
        throw std::logic_error("Read operation called on write-only variable.");
      }

      void postRead() override {
        throw std::logic_error("Read operation called on write-only variable.");
      }

      bool write(ChimeraTK::VersionNumber versionNumber={}) override {
        bool dataLost = false;
        boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> firstSlave;   // will have the data for the other slaves after swapping
        for(auto &slave : FanOut<UserType>::slaves) {     // send out copies to slaves
          if(slave->getNumberOfSamples() != 0) {          // do not send copy if no data is expected (e.g. trigger)
            if(!firstSlave) {                             // in case of first slave, swap instead of copy
              firstSlave = slave;
              firstSlave->accessChannel(0).swap(mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0]);
            }
            else {                                // not the first slave: copy the data from the first slave
              slave->accessChannel(0) = firstSlave->accessChannel(0);
            }
          }
          bool ret = slave->write(versionNumber);
          if(ret) dataLost = true;
        }
        // swap back the data from the first slave so we still have it available
        if(firstSlave) {
          firstSlave->accessChannel(0).swap(mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0]);
        }
        return dataLost;
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

#endif /* CHIMERATK_FEEDING_FAN_OUT_H */

