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
#include "ImplementationAdapter.h"

namespace ChimeraTK {


  /** @todo TODO This class should be split into two classes, one with a thread and the other without. The
   *  threaded version doesn't have to be a NDRegisterAccessor! Instead it should be unified with the
   *  ImplementationAdapter class... */
  template<typename UserType>
  class FanOut : public mtca4u::NDRegisterAccessor<UserType>, public ImplementationAdapterBase {

    public:

      /** Use this constructor if the FanOut should be a consuming implementation. */
      FanOut(boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> feedingImpl)
      : _direction(VariableDirection::consuming)
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

      /** Use this constructor if the FanOut should be a feeding implementation. */
      FanOut()
      : impl(nullptr), _direction(VariableDirection::feeding)
      {}

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
        assert(_direction == VariableDirection::consuming);
        externalTrigger = externalTriggerImpl;
        hasExternalTrigger = true;
      }

      void activate() {
        assert(_direction == VariableDirection::consuming);
        assert(!_thread.joinable());
        _thread = boost::thread([this] { this->run(); });
      }

      void deactivate() {
        if(_thread.joinable()) {
          _thread.interrupt();
          _thread.join();
        }
        assert(!_thread.joinable());
      }

      /** Synchronise feeder and the consumers. This function is executed in the separate thread. */
      void run() {
        assert(_direction == VariableDirection::consuming);
        while(true) {
          boost::this_thread::yield();
          boost::this_thread::interruption_point();
          if(hasExternalTrigger) {
            // wait for external trigger (if present)
            /// @todo TODO replace with proper blocking implementation when supported by the CSA
            while(externalTrigger->readNonBlocking() == false) {
              boost::this_thread::yield();
              boost::this_thread::interruption_point();
            }
            // receive data
            impl->readNonBlocking();
          }
          else {
            // receive data
            while(impl->readNonBlocking() == false) {
              boost::this_thread::yield();
              boost::this_thread::interruption_point();
            }
          }
          boost::this_thread::yield();
          boost::this_thread::interruption_point();
          for(auto &slave : slaves) {     // send out copies to slaves
            // do not send copy if no data is expected (e.g. trigger)
            if(slave->getNumberOfSamples() != 0) {
              slave->accessChannel(0) = impl->accessChannel(0);
            }
            boost::this_thread::yield();
            boost::this_thread::interruption_point();
            slave->write();
          }
        }
      }

      void set(mtca4u::NDRegisterAccessor<UserType> const & other) {
        impl->set(other.get());
      }

      void set(UserType const & t) {
        impl->set(t);
      }

      operator UserType() const {
        return impl->get();
      }

      UserType get() const {
        return impl->get();
      }

      const std::type_info& getValueType() const {
        return typeid(UserType);
      }

      bool isReadable() const {
        return _direction == VariableDirection::consuming;
      }
      
      bool isReadOnly() const {
        return _direction == VariableDirection::consuming;
      }
      
      bool isWriteable() const {
        return _direction == VariableDirection::feeding;
      }

      TimeStamp getTimeStamp() const {
        return impl->getTimeStamp();
      }
      
      void read() {
        throw std::logic_error("Blocking read is not supported by process array.");
      }
      
      bool readNonBlocking() {
        bool ret = impl->readNonBlocking();
        if(ret) {
          mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0].swap(impl->accessChannel(0));
          for(auto &slave : slaves) {     // send out copies to slaves
            // do not send copy if no data is expected (e.g. trigger)
            if(slave->getNumberOfSamples() != 0) {
              slave->accessChannel(0) = mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0];
            }
            slave->write();
          }
        }
        return ret;
      }

      void write() {
        for(auto &slave : slaves) {     // send out copies to slaves
          // do not send copy if no data is expected (e.g. trigger)
          if(slave->getNumberOfSamples() != 0) {
            slave->accessChannel(0) = mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0];
          }
          slave->write();
        }
        impl->accessChannel(0).swap(mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0]);
        impl->write();
        impl->accessChannel(0).swap(mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0]);
        return;
      }
      
      virtual bool isSameRegister(const boost::shared_ptr<const mtca4u::TransferElement>& e) const{
        // only true if the very instance of the transfer element is the same
        return e.get() == this;
      }
      
      virtual std::vector<boost::shared_ptr<mtca4u::TransferElement> > getHardwareAccessingElements(){
        return { boost::enable_shared_from_this<mtca4u::TransferElement>::shared_from_this() };
      }
      
      virtual void replaceTransferElement(boost::shared_ptr<mtca4u::TransferElement>){
        // You can't replace anything here. Just do nothing.
      }
      
    protected:

      boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> impl;

      std::list<boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>>> slaves;

      VariableDirection _direction;

      bool hasExternalTrigger{false};
      boost::shared_ptr<mtca4u::TransferElement> externalTrigger;

      /** Thread handling the synchronisation, if needed */
      boost::thread _thread;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_FAN_OUT_H */
