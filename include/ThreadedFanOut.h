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
    ThreadedFanOut(boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> feedingImpl, VariableNetwork& network)
    : FanOut<UserType>(feedingImpl), _network(network) {}

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

      // Read all variables once to obtain the initial values from the devices and from the control system persistency
      // layer.
      if((_network.getFeedingNode().getType() == NodeType::Device ||
             _network.getFeedingNode().getType() == NodeType::Constant) &&
          _network.getConsumingNodes().size() == 1 &&
          _network.getTriggerType() != VariableNetwork::TriggerType::external) {
        // Special case if a push-type device accessor is directly placed into our module (without FanOut): Since the
        // device will not push an initial value to us, a read() would block until the value is changed for the first
        // time. Hence we need to use readLatest() here to obtain the initial value.
        // The same is true for Constant accessors, since their read() never returns.
        FanOut<UserType>::impl->readLatest();
      }
      else if(_network.getFeedingNode().getType() != NodeType::Application) {
        // This case includes inserted FanOuts, which have their own thread, so we must block to make sure to get the
        // initial value. FanOuts will always forward the initial values and the ControlSystemAdapter will always psuh
        // the initial values, hence a blocking read() here will not block indefinitively.
        FanOut<UserType>::impl->read();
      }

      while(true) {
        // send out copies to slaves
        Profiler::startMeasurement();
        boost::this_thread::interruption_point();
        auto validity = FanOut<UserType>::impl->dataValidity();
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
        // receive data
        boost::this_thread::interruption_point();
        Profiler::stopMeasurement();
        FanOut<UserType>::impl->read();
      }
    }

   protected:
    /** Thread handling the synchronisation, if needed */
    boost::thread _thread;

    /** Reference to VariableNetwork which is being realised by this FanOut. **/
    VariableNetwork& _network;
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
