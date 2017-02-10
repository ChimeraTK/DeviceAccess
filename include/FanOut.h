/*
 * FanOut.h
 *
 *  Created on: Jun 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_FAN_OUT_H
#define CHIMERATK_FAN_OUT_H

#include <mtca4u/NDRegisterAccessor.h>

#include "ApplicationException.h"
#include "InternalModule.h"

namespace ChimeraTK {

  
  template<typename UserType>
  class FanOut : public mtca4u::NDRegisterAccessor<UserType>, public InternalModule {

    public:

      /** Use this constructor if the FanOut should be a consuming implementation. */
      FanOut(boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> feedingImpl)
      {
        impl = boost::dynamic_pointer_cast<mtca4u::NDRegisterAccessor<UserType>>(feedingImpl);
        if(!impl) {
          throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>(
              "The FanOut has been constructed with a wrong output implementation type!");
        }
        mtca4u::NDRegisterAccessor<UserType>::buffer_2D.resize( impl->getNumberOfChannels() );
        for(size_t i=0; i<impl->getNumberOfChannels(); i++) {
          mtca4u::NDRegisterAccessor<UserType>::buffer_2D[i].resize( impl->getNumberOfSamples() );
        }
      }

      /** If activate() has been called on this FanOut, deactivate() must be called before destruction. Otherweise
       *  an assertion will be raised.
       *  Design note: stopping the thread inside the destructor may be too late, since the thread will be accessing
       *  the object while it is being destroyed already! */
      ~FanOut() {
        assert(!_thread.joinable());
      }

      /** Add a slave to the FanOut. Only sending end-points of a consuming node may be added. */
      void addSlave(boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> slave) {
        if(!slave->isWriteable()) {
          throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>(
              "FanOut::addSlave() has been called with a receiving implementation!");
        }
        auto castedSlave = boost::dynamic_pointer_cast<mtca4u::NDRegisterAccessor<UserType>>(slave);
        if(!castedSlave) {
          throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>(
              "FanOut::addSlave() has been called with a wrong input implementation type!");
        }
        if(impl == nullptr) {       // the first slave will be used as a "main" implementation, if
          impl = castedSlave;       // none was specified at construction
          mtca4u::NDRegisterAccessor<UserType>::buffer_2D.resize( impl->getNumberOfChannels() );
          for(size_t i=0; i<impl->getNumberOfChannels(); i++) {
            mtca4u::NDRegisterAccessor<UserType>::buffer_2D[i].resize( impl->getNumberOfSamples() );
          }
        }
        else {
          // check if array shape is compatible, unless the receiver is a trigger node, so no data is expected
          if( castedSlave->getNumberOfSamples() != 0 && 
              ( castedSlave->getNumberOfChannels() != impl->getNumberOfChannels() ||
                castedSlave->getNumberOfSamples() != impl->getNumberOfSamples()      ) ) {
            std::string what = "FanOut::addSlave(): Trying to add a slave '";
            what += castedSlave->getName();
            what += "' with incompatible array shape! Name of master: ";
            what += impl->getName();
            throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>(what.c_str());
          }
          slaves.push_back(castedSlave);
        }
      }
        
      /** Add an external trigger to allow poll-type feeders to be used for push-type consumers */
      void addExternalTrigger(const boost::shared_ptr<mtca4u::TransferElement>& externalTriggerImpl) {
        externalTrigger = externalTriggerImpl;
        hasExternalTrigger = true;
      }

      void activate() override {
        assert(!_thread.joinable());
        _thread = boost::thread([this] { this->run(); });
      }

      void deactivate() override {
        if(_thread.joinable()) {
          _thread.interrupt();
          _thread.join();
        }
        assert(!_thread.joinable());
      }

      /** Synchronise feeder and the consumers. This function is executed in the separate thread. */
      void run() {
        while(true) {
          boost::this_thread::interruption_point();
          if(hasExternalTrigger) {
            // wait for external trigger (if present)
            externalTrigger->read();
            // receive data
            impl->readNonBlocking();
          }
          else {
            // receive data
            impl->read();
          }
          for(auto &slave : slaves) {     // send out copies to slaves
            // do not send copy if no data is expected (e.g. trigger)
            if(slave->getNumberOfSamples() != 0) {
              slave->accessChannel(0) = impl->accessChannel(0);
            }
            slave->write();
          }
        }
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

      TimeStamp getTimeStamp() const override {
        return impl->getTimeStamp();
      }
      
      void doReadTransfer() override {
        impl->read();
      }
      
      bool doReadTransferNonBlocking() override {
        return impl->readNonBlocking();
      }
      
      void postRead() override {
        mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0].swap(impl->accessChannel(0));
        for(auto &slave : slaves) {     // send out copies to slaves
          // do not send copy if no data is expected (e.g. trigger)
          if(slave->getNumberOfSamples() != 0) {
            slave->accessChannel(0) = mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0];
          }
          slave->write();
        }
      }

      void write() override {
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
      
    protected:

      boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> impl;

      std::list<boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>>> slaves;

      bool hasExternalTrigger{false};
      boost::shared_ptr<mtca4u::TransferElement> externalTrigger;

      /** Thread handling the synchronisation, if needed */
      boost::thread _thread;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_FAN_OUT_H */
