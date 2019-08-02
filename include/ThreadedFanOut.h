/*
 * ThreadedFanOut.h
 *
 *  Created on: Jun 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_THREADED_FAN_OUT_H
#define CHIMERATK_THREADED_FAN_OUT_H

#include <ChimeraTK/NDRegisterAccessor.h>
#include <ChimeraTK/ReadAnyGroup.h>

#include "Application.h"
#include "FanOut.h"
#include "InternalModule.h"

namespace ChimeraTK {

  /** FanOut implementation with an internal thread which waits for new data which
   * is read from the given feeding implementation and distributed to any number
   * of slaves. */
  template<typename UserType>
  class ThreadedFanOut : public FanOut<UserType>, public InternalModule {
   public:
    ThreadedFanOut(boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> feedingImpl)
    : FanOut<UserType>(feedingImpl) {}

    ~ThreadedFanOut() { deactivate(); }

    void activate() override {
      assert(!_thread.joinable());
      _thread = boost::thread([this] { this->run(); });
    }

    void deactivate() override {
      if(_thread.joinable()) {
        _thread.interrupt();
        FanOut<UserType>::interrupt();
        _thread.join();
      }
      assert(!_thread.joinable());
    }

    /** Synchronise feeder and the consumers. This function is executed in the
     * separate thread. */
    virtual void run() {
      Application::registerThread("ThFO" + FanOut<UserType>::impl->getName());
      Application::testableModeLock("start");

      while(true) {
        // receive data
        boost::this_thread::interruption_point();
        Profiler::stopMeasurement();
        FanOut<UserType>::impl->read();
        Profiler::startMeasurement();
        boost::this_thread::interruption_point();
        auto validity = FanOut<UserType>::impl->dataValidity();
        // send out copies to slaves
        auto version = FanOut<UserType>::impl->getVersionNumber();
        for(auto& slave : FanOut<UserType>::slaves) {
          // do not send copy if no data is expected (e.g. trigger)
          if(slave->getNumberOfSamples() != 0) {
            slave->accessChannel(0) = FanOut<UserType>::impl->accessChannel(0);
          }
          slave->setDataValidity(validity);
          bool dataLoss = slave->writeDestructively(version);
          if(dataLoss) Application::incrementDataLossCounter();
        }
      }
    }

   protected:
    /** Thread handling the synchronisation, if needed */
    boost::thread _thread;
  };

  /********************************************************************************************************************/

  /** Same as ThreadedFanOut but with return channel */
  template<typename UserType>
  class ThreadedFanOutWithReturn : public ThreadedFanOut<UserType> {
   public:
    using ThreadedFanOut<UserType>::ThreadedFanOut;

    void setReturnChannelSlave(boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> returnChannelSlave) {
      _returnChannelSlave = returnChannelSlave;
    }

    void addSlave(
        boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> slave, VariableNetworkNode& consumer) override {
      FanOut<UserType>::addSlave(slave, consumer);
      if(consumer.getDirection().withReturn) {
        assert(_returnChannelSlave == nullptr);
        _returnChannelSlave = slave;
      }
    }

    void run() override {
      Application::registerThread("ThFO" + FanOut<UserType>::impl->getName());
      Application::testableModeLock("start");

      ReadAnyGroup group({FanOut<UserType>::impl, _returnChannelSlave});
      while(true) {
        // receive data
        boost::this_thread::interruption_point();
        Profiler::stopMeasurement();
        auto var = group.readAny();
        Profiler::startMeasurement();
        boost::this_thread::interruption_point();
        // if the update came through the return channel, return it to the feeder
        if(var == _returnChannelSlave->getId()) {
          FanOut<UserType>::impl->accessChannel(0).swap(_returnChannelSlave->accessChannel(0));
          FanOut<UserType>::impl->writeDestructively(_returnChannelSlave->getVersionNumber());
        }
        // send out copies to slaves
        auto version = FanOut<UserType>::impl->getVersionNumber();
        for(auto& slave : FanOut<UserType>::slaves) {
          // do not feed back value returnChannelSlave if it was received from it
          if(slave->getId() == var) continue;
          // do not send copy if no data is expected (e.g. trigger)
          if(slave->getNumberOfSamples() != 0) {
            slave->accessChannel(0) = FanOut<UserType>::impl->accessChannel(0);
          }
          bool dataLoss = slave->writeDestructively(version);
          if(dataLoss) Application::incrementDataLossCounter();
        }
      }
    }

   protected:
    /** Thread handling the synchronisation, if needed */
    boost::thread _thread;

    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> _returnChannelSlave;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_THREADED_FAN_OUT_H */
