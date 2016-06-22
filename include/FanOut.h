/*
 * FanOut.h
 *
 *  Created on: Jun 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_FAN_OUT_H
#define CHIMERATK_FAN_OUT_H

#include <ControlSystemAdapter/ProcessScalar.h>

#include "ApplicationException.h"
#include "ImplementationAdapter.h"

namespace ChimeraTK {


  /** @todo TODO This class should be split into two classes, one with a thread and the other without. The
   *  threaded version doesn't have to be a ProcessScalar! Instead it should be unified with the
   *  ImplementationAdapter class... */
  template<typename UserType>
  class FanOut : public mtca4u::ProcessScalar<UserType>, public ImplementationAdapterBase {

    public:

      /** Use this constructor if the FanOut should be a consuming implementation. */
      FanOut(boost::shared_ptr<mtca4u::ProcessVariable> feedingImpl)
      : _direction(VariableDirection::consuming)
      {
        impl = boost::dynamic_pointer_cast<mtca4u::ProcessScalar<UserType>>(feedingImpl);
        if(!impl) {
          throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>(
              "The FanOut has been constructed with a wrong output implementation type!");
        }
      }

      /** Use this constructor if the FanOut should be a feeding implementation. */
      FanOut()
      : impl(nullptr), _direction(VariableDirection::feeding)
      {}

      /** The destructor will terminate the internal thread, if running */
      ~FanOut() {
        if(_thread.joinable()) {
          requestTerminateThread = true;
          _thread.join();
        }
      }

      /** Add a slave to the FanOut. Only sending end-points of a consuming node may be added. */
      void addSlave(boost::shared_ptr<mtca4u::ProcessVariable> slave) {
        if(!slave->isSender()) {
          throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>(
              "FanOut::addSlave() has been called with a receiving implementation!");
        }
        auto castedSlave = boost::dynamic_pointer_cast<mtca4u::ProcessScalar<UserType>>(slave);
        if(!castedSlave) {
          throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>(
              "FanOut::addSlave() has been called with a wrong input implementation type!");
        }
        if(impl == nullptr) {       // the first slave will be used as a "main" implementation, if
          impl = castedSlave;       // none was specified at construction
        }
        else {
          slaves.push_back(castedSlave);
        }
      }

      /** Add an external trigger to allow poll-type feeders to be used for push-type consumers */
      void addExternalTrigger(const boost::shared_ptr<mtca4u::ProcessVariable>& externalTriggerImpl) {
        assert(_direction == VariableDirection::consuming);
        externalTrigger = externalTriggerImpl;
        hasExternalTrigger = true;
      }

      void activate() {
        assert(_direction == VariableDirection::consuming);
        assert(!_thread.joinable());
        _thread = std::thread([this] { this->run(); });
      }

      /** Synchronise feeder and the consumers. This function is executed in the separate thread. */
      void run() {
        assert(_direction == VariableDirection::consuming);
        assert(hasExternalTrigger);
        while(true) {
          // wait for external trigger
          /// @todo TODO replace with proper blocking implementation when supported by the CSA
          while(externalTrigger->receive() == false) {
            if(requestTerminateThread) return;
            std::this_thread::yield();
          }
          // receive data
          impl->receive();
          for(auto &slave : slaves) {     // send out copies to slaves
            slave->set(impl->get());
            slave->send();
          }
        }
      }

      void set(mtca4u::ProcessScalar<UserType> const & other) {
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

      bool isReceiver() const {
        return _direction == VariableDirection::consuming;
      }

      bool isSender() const {
        return _direction == VariableDirection::feeding;
      }

      TimeStamp getTimeStamp() const {
        return impl->getTimeStamp();
      }

      bool receive() {
        bool ret = impl->receive();
        if(ret) {
          for(auto &slave : slaves) {     // send out copies to slaves
            slave->set(impl->get());
            slave->send();
          }
        }
        return ret;
      }

      bool send() {
        bool ret = true;
        for(auto &slave : slaves) {     // send out copies to slaves
          slave->set(impl->get());
          bool ret2 = slave->send();
          if(!ret2) ret = false;
        }
        bool ret2 = impl->send();
        if(!ret2) ret = false;
        return ret;
      }

    protected:

      boost::shared_ptr<mtca4u::ProcessScalar<UserType>> impl;

      std::list<boost::shared_ptr<mtca4u::ProcessScalar<UserType>>> slaves;

      VariableDirection _direction;

      bool hasExternalTrigger{false};
      boost::shared_ptr<mtca4u::ProcessVariable> externalTrigger;

      /** Thread handling the synchronisation, if needed */
      std::thread _thread;

      /** Flag to request termination of the synchronisation thread. */
      bool requestTerminateThread{false};

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_FAN_OUT_H */
