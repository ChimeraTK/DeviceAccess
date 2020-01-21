/*
 * TriggerFanOut.h
 *
 *  Created on: Jun 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_TRIGGER_FAN_OUT_H
#define CHIMERATK_TRIGGER_FAN_OUT_H

#include <ChimeraTK/NDRegisterAccessor.h>
#include <ChimeraTK/SupportedUserTypes.h>
#include <ChimeraTK/TransferGroup.h>

#include "Application.h"
#include "FeedingFanOut.h"
#include "InternalModule.h"
#include "Profiler.h"
#include "DeviceModule.h"

constexpr useconds_t DeviceOpenTimeout = 500;

namespace ChimeraTK {

  /** InternalModule which waits for a trigger, then reads a number of variables
   * and distributes each of them to any number of slaves. */
  class TriggerFanOut : public InternalModule {
   public:
    TriggerFanOut(const boost::shared_ptr<ChimeraTK::TransferElement>& externalTriggerImpl, DeviceModule& deviceModule)
    : externalTrigger(externalTriggerImpl), _deviceModule(deviceModule) {}

    ~TriggerFanOut() { deactivate(); }

    void activate() override {
      assert(!_thread.joinable());
      _thread = boost::thread([this] { this->run(); });
    }

    void deactivate() override {
      if(_thread.joinable()) {
        _thread.interrupt();
        externalTrigger->interrupt();
        _deviceModule.notify();
        _thread.join();
      }
      assert(!_thread.joinable());
    }

    /** Add a new network the TriggerFanOut. The network is defined by its feeding
     * node. This function will return the corresponding FeedingFanOut, to which
     * all slaves have to be added. */
    template<typename UserType>
    boost::shared_ptr<FeedingFanOut<UserType>> addNetwork(
        boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> feedingNode) {
      assert(feedingNode.get() != nullptr);
      transferGroup.addAccessor(feedingNode);
      auto feedingFanOut = boost::make_shared<FeedingFanOut<UserType>>(feedingNode->getName(), feedingNode->getUnit(),
          feedingNode->getDescription(), feedingNode->getNumberOfSamples(),
          false); // in TriggerFanOuts we cannot have return channels
      boost::fusion::at_key<UserType>(fanOutMap.table)[feedingNode] = feedingFanOut;
      return feedingFanOut;
    }

    /** Synchronise feeder and the consumers. This function is executed in the
     * separate thread. */
    void run() {
      Application::registerThread("TrFO" + externalTrigger->getName());
      Application::testableModeLock("start");
      while(true) {
        // wait for external trigger
        boost::this_thread::interruption_point();
        Profiler::stopMeasurement();
        externalTrigger->read();
        Profiler::startMeasurement();
        boost::this_thread::interruption_point();
        // receive data. We need to catch exceptions here, since the ExceptionHandlingDecorator cannot do this for us
        // inside a TransferGroup, if the exception is thrown inside doReadTransfer() (as it is directly called on the
        // lowest-level TransferElements inside the group).
        auto lastValidity = DataValidity::ok;
      retry:
        try {
          if(!_deviceModule.device.isOpened()) {
            //auto version = externalTrigger->getVersionNumber();
            //boost::fusion::for_each(fanOutMap.table, SendDataToConsumers(version, DataValidity::faulty));
            Application::getInstance().testableModeUnlock("waitForDeviceOpen");
            boost::this_thread::sleep(boost::posix_time::millisec(DeviceOpenTimeout));
            Application::getInstance().testableModeLock("waitForDeviceOpen");
            goto retry;
          }
          transferGroup.read();
        }
        catch(ChimeraTK::runtime_error& e) {
          // send the data to the consumers
          auto version = externalTrigger->getVersionNumber();
          if(lastValidity == DataValidity::ok) {
            lastValidity = DataValidity::faulty;
            boost::fusion::for_each(fanOutMap.table, SendDataToConsumers(version, lastValidity));
          }
          _deviceModule.reportException(e.what());
          goto retry;
        }
        // send the data to the consumers
        auto version = externalTrigger->getVersionNumber();
        boost::fusion::for_each(fanOutMap.table, SendDataToConsumers(version));
      }
    }

   protected:
    /** Functor class to send data to the consumers, suitable for
     * boost::fusion::for_each(). */
    struct SendDataToConsumers {
      SendDataToConsumers(VersionNumber version, DataValidity validity = DataValidity::ok)
      : _version(version), _validity(validity) {}

      template<typename PAIR>
      void operator()(PAIR& pair) const {
        auto theMap = pair.second; // map of feeder to FeedingFanOut (i.e. part of
                                   // the fanOutMap)

        // iterate over all feeder/FeedingFanOut pairs
        for(auto& network : theMap) {
          auto feeder = network.first;
          auto fanOut = network.second;
          fanOut->setDataValidity(_validity);
          fanOut->accessChannel(0).swap(feeder->accessChannel(0));
          bool dataLoss = fanOut->writeDestructively(_version);
          if(dataLoss) Application::incrementDataLossCounter();
          // no need to swap back since we don't need the data
        }
      }

      VersionNumber _version;
      DataValidity _validity;
    };

    /** TransferElement acting as our trigger */
    boost::shared_ptr<ChimeraTK::TransferElement> externalTrigger;

    /** Map of the feeding NDRegisterAccessor to the corresponding FeedingFanOut
     * for each UserType */
    template<typename UserType>
    using FanOutMap = std::map<boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>>,
        boost::shared_ptr<FeedingFanOut<UserType>>>;
    TemplateUserTypeMap<FanOutMap> fanOutMap;

    /** TransferGroup containing all feeders NDRegisterAccessors */
    ChimeraTK::TransferGroup transferGroup;

    /** Thread handling the synchronisation, if needed */
    boost::thread _thread;

    /** The DeviceModule of the feeder. Required for exception handling */
    DeviceModule& _deviceModule;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_TRIGGER_FAN_OUT_H */
