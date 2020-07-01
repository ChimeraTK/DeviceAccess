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
    TriggerFanOut(const boost::shared_ptr<ChimeraTK::TransferElement>& externalTriggerImpl, DeviceModule& deviceModule,
        VariableNetwork& network)
    : externalTrigger(externalTriggerImpl), _deviceModule(deviceModule), _network(network) {}

    ~TriggerFanOut() { deactivate(); }

    void activate() override {
      assert(!_thread.joinable());
      _thread = boost::thread([this] { this->run(); });

      // Wait until the thread has launched and acquired and released the testable mode lock at least once.
      if(Application::getInstance().isTestableModeEnabled()) {
        while(!testableModeReached) {
          Application::getInstance().testableModeUnlock("releaseForReachTestableMode");
          usleep(100);
          Application::getInstance().testableModeLock("acquireForReachTestableMode");
        }
      }
    }

    void deactivate() override {
      if(_thread.joinable()) {
        _thread.interrupt();
        if(externalTrigger->getAccessModeFlags().has(AccessMode::wait_for_new_data)) {
          externalTrigger->interrupt();
        }
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
      testableModeReached = true;

      ChimeraTK::VersionNumber version = Application::getInstance().getStartVersion();

      // If trigger gets an initial value pushed, read it (otherwise we would trigger twice at application start)
      auto hasInitialValue = _network.getFeedingNode().getExternalTrigger().hasInitialValue();
      if(hasInitialValue == VariableNetworkNode::InitialValueMode::Push) {
        externalTrigger->read();
        version = externalTrigger->getVersionNumber();
      }

      while(true) {
        // receive data. We need to catch exceptions here, since the ExceptionHandlingDecorator cannot do this for us
        // inside a TransferGroup, if the exception is thrown inside doReadTransfer() (as it is directly called on the
        // lowest-level TransferElements inside the group).
        auto lastValidity = DataValidity::ok;
      retry:
        try {
          if(!_deviceModule.device.isOpened()) {
            Application::getInstance().testableModeUnlock("waitForDeviceOpen");
            boost::this_thread::sleep(boost::posix_time::millisec(DeviceOpenTimeout));
            Application::getInstance().testableModeLock("waitForDeviceOpen");
            goto retry;
          }
          transferGroup.read();
        }
        catch(ChimeraTK::runtime_error& e) {
          // send the data to the consumers
          if(lastValidity == DataValidity::ok) {
            lastValidity = DataValidity::faulty;
            boost::fusion::for_each(fanOutMap.table, SendDataToConsumers(version, lastValidity));
          }
          _deviceModule.reportException(e.what());
          //_deviceModule.waitForRecovery();
          goto retry;
        }
        // send the version number to the consumers
        boost::fusion::for_each(fanOutMap.table, SendDataToConsumers(version));
        // wait for external trigger (exception handling is done here by the decorator)
        boost::this_thread::interruption_point();
        Profiler::stopMeasurement();
        externalTrigger->read();
        Profiler::startMeasurement();
        boost::this_thread::interruption_point();
        version = externalTrigger->getVersionNumber();
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

    /** Reference to VariableNetwork which is being realised by this FanOut. **/
    VariableNetwork& _network;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_TRIGGER_FAN_OUT_H */
