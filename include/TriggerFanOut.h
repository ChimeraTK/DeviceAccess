/*
 * TriggerFanOut.h
 *
 *  Created on: Jun 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_TRIGGER_FAN_OUT_H
#define CHIMERATK_TRIGGER_FAN_OUT_H

#include <mtca4u/NDRegisterAccessor.h>
#include <mtca4u/SupportedUserTypes.h>
#include <mtca4u/TransferGroup.h>

#include "Application.h"
#include "FeedingFanOut.h"
#include "InternalModule.h"

namespace ChimeraTK {
  
  /** InternalModule which waits for a trigger, then reads a number of variables and distributes each of them to any
   *  number of slaves. */
  class TriggerFanOut : public InternalModule {

    public:

      TriggerFanOut(const boost::shared_ptr<mtca4u::TransferElement>& externalTriggerImpl)
      : externalTrigger(externalTriggerImpl)
      {}
      
      ~TriggerFanOut() {
        deactivate();
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

      /** Add a new network the TriggerFanOut. The network is defined by its feeding node. This function will return
       *  the corresponding FeedingFanOut, to which all slaves have to be added. */
      template<typename UserType>
      boost::shared_ptr<FeedingFanOut<UserType>> addNetwork(boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> feedingNode) {
        transferGroup.addAccessor(*feedingNode);
        auto feedingFanOut = boost::make_shared<FeedingFanOut<UserType>>( feedingNode->getName(), feedingNode->getUnit(),
            feedingNode->getDescription(), feedingNode->getNumberOfSamples() );
        boost::fusion::at_key<UserType>(fanOutMap.table)[feedingNode] = feedingFanOut;
        return feedingFanOut;
      }

      /** Synchronise feeder and the consumers. This function is executed in the separate thread. */
      void run() {
        Application::getInstance().testableModeThreadName() = "TriggerFanOut "+externalTrigger->getName();
        Application::testableModeLock("start");
        while(true) {
          // wait for external trigger
          boost::this_thread::interruption_point();
          externalTrigger->read();
          boost::this_thread::interruption_point();
          // receive data
          transferGroup.read();
          // send the data to the consumers
          boost::fusion::for_each(fanOutMap.table, SendDataToConsumers());
        }
      }
      
    protected:
      
      /** Functor class to send data to the consumers, suitable for boost::fusion::for_each(). */
      struct SendDataToConsumers {
        template<typename PAIR>
        void operator()(PAIR &pair) const {
          
          auto theMap = pair.second;    // map of feeder to FeedingFanOut (i.e. part of the fanOutMap)
          
          // iterate over all feeder/FeedingFanOut pairs
          for(auto &network : theMap) {
            auto feeder = network.first;
            auto fanOut = network.second;
            fanOut->accessChannel(0).swap(feeder->accessChannel(0));
            fanOut->write();
            // no need to swap back since we don't need the data
          }
          
        }
      };

      /** TransferElement acting as our trigger */
      boost::shared_ptr<mtca4u::TransferElement> externalTrigger;

      /** Map of the feeding NDRegisterAccessor to the corresponding FeedingFanOut for each UserType */
      template<typename UserType>
      using FanOutMap = std::map<boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>>, boost::shared_ptr<FeedingFanOut<UserType>>>;
      TemplateUserTypeMap<FanOutMap> fanOutMap;
      
      /** TransferGroup containing all feeders NDRegisterAccessors */
      mtca4u::TransferGroup transferGroup;

      /** Thread handling the synchronisation, if needed */
      boost::thread _thread;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_TRIGGER_FAN_OUT_H */


