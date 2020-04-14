/*
 * FeedingFanOut.h
 *
 *  Created on: Jun 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_FEEDING_FAN_OUT_H
#define CHIMERATK_FEEDING_FAN_OUT_H

#include <ChimeraTK/NDRegisterAccessor.h>

#include "FanOut.h"

namespace ChimeraTK {

  /**
   * NDRegisterAccessor implementation which distributes values written to this
   * accessor out to any number of slaves.
   */
  template<typename UserType>
  class FeedingFanOut : public FanOut<UserType>, public ChimeraTK::NDRegisterAccessor<UserType> {
   public:
    FeedingFanOut(std::string const& name, std::string const& unit, std::string const& description,
        size_t numberOfElements, bool withReturn)
    : FanOut<UserType>(boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>>()),
      ChimeraTK::NDRegisterAccessor<UserType>("FeedingFanOut:" + name, unit, description), _withReturn(withReturn) {
      ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D.resize(1);
      ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D[0].resize(numberOfElements);
    }

    /** Add a slave to the FanOut. Only sending end-points of a consuming node may
     * be added. */
    void addSlave(boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> slave, VariableNetworkNode&) override {
      // check if array shape is compatible, unless the receiver is a trigger
      // node, so no data is expected
      if(slave->getNumberOfSamples() != 0 &&
          (slave->getNumberOfChannels() != 1 || slave->getNumberOfSamples() != this->getNumberOfSamples())) {
        std::string what = "FeedingFanOut::addSlave(): Trying to add a slave '" + slave->getName();
        what += "' with incompatible array shape! Name of fan out: '" + this->getName() + "'";
        throw ChimeraTK::logic_error(what.c_str());
      }

      // make sure slave is writeable
      if(!slave->isWriteable()) {
        throw ChimeraTK::logic_error("FeedingFanOut::addSlave() has been called "
                                     "with a receiving implementation!");
      }

      // handle return channels
      if(_withReturn) {
        if(slave->isReadable()) {
          if(_hasReturnSlave) {
            throw ChimeraTK::logic_error("FeedingFanOut: Cannot add multiple slaves with return channel!");
          }
          _hasReturnSlave = true;
          _returnSlave = slave;
        }
      }

      // add the slave
      FanOut<UserType>::slaves.push_back(slave);
    }

    bool isReadable() const override { return _withReturn; }

    bool isReadOnly() const override { return false; }

    bool isWriteable() const override { return true; }

    void doReadTransfer() override {
      if(!_withReturn) throw ChimeraTK::logic_error("Read operation called on write-only variable.");
      _returnSlave->readTransfer();
    }

    bool doReadTransferNonBlocking() override {
      if(!_withReturn) throw ChimeraTK::logic_error("Read operation called on write-only variable.");
      return _returnSlave->readTransferNonBlocking();
    }

    bool doReadTransferLatest() override {
      if(!_withReturn) throw ChimeraTK::logic_error("Read operation called on write-only variable.");
      return _returnSlave->readTransferLatest();
    }

    void doPreRead(TransferType type) override {
      if(!_withReturn) throw ChimeraTK::logic_error("Read operation called on write-only variable.");
      _returnSlave->accessChannel(0).swap(ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D[0]);
      _returnSlave->preRead(type);
    }

    void doPostRead(TransferType type) override {
      if(!_withReturn) throw ChimeraTK::logic_error("Read operation called on write-only variable.");
      assert(_hasReturnSlave);
      _returnSlave->postRead(type);
      _returnSlave->accessChannel(0).swap(ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D[0]);
      // distribute return-channel update to the other slaves
      for(auto& slave : FanOut<UserType>::slaves) { // send out copies to slaves
        if(slave == _returnSlave) continue;
        if(slave->getNumberOfSamples() != 0) { // do not send copy if no data is expected (e.g. trigger)
          slave->accessChannel(0) = ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D[0];
        }
        slave->writeDestructively(_returnSlave->getVersionNumber());
      }
    }

    ChimeraTK::TransferFuture doReadTransferAsync() override {
      if(!_withReturn) throw ChimeraTK::logic_error("Read operation called on write-only variable.");
      return {_returnSlave->readTransferAsync(), this};
    }

    void doPreWrite(TransferType type) override {
      for(auto& slave : FanOut<UserType>::slaves) {       // send out copies to slaves
        if(slave->getNumberOfSamples() != 0) {            // do not send copy if no data is expected (e.g. trigger)
          if(slave == FanOut<UserType>::slaves.front()) { // in case of first slave, swap instead of copy
            slave->accessChannel(0).swap(ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D[0]);
          }
          else { // not the first slave: copy the data from the first slave
            slave->accessChannel(0) = FanOut<UserType>::slaves.front()->accessChannel(0);
          }
        }
        slave->setDataValidity(this->dataValidity());
      }
      // pre write may only be called on the target accessors after we have filled
      // them all, otherwise the first accessor might take us the data away...
      for(auto& slave : FanOut<UserType>::slaves) {
        slave->preWrite(type);
      }
    }

    bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber = {}) override {
      bool dataLost = false;
      bool isFirst = true;
      for(auto& slave : FanOut<UserType>::slaves) {
        bool ret;
        if(isFirst) {
          isFirst = false;
          ret = slave->writeTransfer(versionNumber);
        }
        else {
          ret = slave->writeTransferDestructively(versionNumber);
        }
        if(ret) dataLost = true;
      }
      return dataLost;
    }

    bool doWriteTransferDestructively(ChimeraTK::VersionNumber versionNumber = {}) override {
      bool dataLost = false;
      for(auto& slave : FanOut<UserType>::slaves) {
        bool ret = slave->writeTransferDestructively(versionNumber);
        if(ret) dataLost = true;
      }
      return dataLost;
    }

    void doPostWrite(TransferType type) override {
      for(auto& slave : FanOut<UserType>::slaves) {
        slave->postWrite(type);
      }
      FanOut<UserType>::slaves.front()->accessChannel(0).swap(ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D[0]);
    }

    bool mayReplaceOther(const boost::shared_ptr<const ChimeraTK::TransferElement>&) const override {
      return false; /// @todo implement properly?
    }

    std::list<boost::shared_ptr<ChimeraTK::TransferElement>> getInternalElements() override {
      return {}; /// @todo implement properly?
    }

    std::vector<boost::shared_ptr<ChimeraTK::TransferElement>> getHardwareAccessingElements() override {
      return {boost::enable_shared_from_this<ChimeraTK::TransferElement>::shared_from_this()}; /// @todo implement
                                                                                               /// properly?
    }

    void replaceTransferElement(boost::shared_ptr<ChimeraTK::TransferElement>) override {
      // You can't replace anything here. Just do nothing.
      /// @todo implement properly?
    }

    AccessModeFlags getAccessModeFlags() const override { return {AccessMode::wait_for_new_data}; }

    VersionNumber getVersionNumber() const override { return FanOut<UserType>::slaves.front()->getVersionNumber(); }

    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> getReturnSlave() { return _returnSlave; }

    void setDataValidity(DataValidity valid = DataValidity::ok) override { validity = valid; }

    DataValidity dataValidity() const override { return validity; }

    void interrupt() override {
      // call the interrut sequences of the fan out (interrupts for fan input and all outputs), and the ndRegisterAccessor
      FanOut<UserType>::interrupt();
      ChimeraTK::NDRegisterAccessor<UserType>::interrupt();
    }

   protected:
    /// Flag whether this FeedingFanOut has a return channel. Is specified in the
    /// constructor
    bool _withReturn;

    /// Used if _withReturn is true: flag whether the corresponding slave with the
    /// return channel has already been added.
    bool _hasReturnSlave{false};

    /// The slave with return channel
    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> _returnSlave;

    /// DataValidity to attach to the data
    DataValidity validity{DataValidity::ok};
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_FEEDING_FAN_OUT_H */
