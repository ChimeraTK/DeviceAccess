/*
 * ThreadedFanOut.h
 *
 *  Created on: Jun 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_THREADED_FAN_OUT_H
#define CHIMERATK_THREADED_FAN_OUT_H

#include <mtca4u/NDRegisterAccessor.h>

#include "FanOut.h"
#include "InternalModule.h"

namespace ChimeraTK {
  
  /** FanOut implementation with an internal thread which waits for new data which is read from the given feeding
   *  implementation and distributed to any number of slaves. */
  template<typename UserType>
  class ThreadedFanOut : public FanOut<UserType>, public InternalModule {

    public:

      ThreadedFanOut(boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> feedingImpl)
      : FanOut<UserType>(feedingImpl)
      {}
      

      /** If activate() has been called on this FanOut, deactivate() must be called before destruction. Otherweise
       *  an assertion will be raised.
       *  Design note: stopping the thread inside the destructor may be too late, since the thread will be accessing
       *  the object while it is being destroyed already! */
      ~ThreadedFanOut() {
        assert(!_thread.joinable());
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
            FanOut<UserType>::impl->readNonBlocking();
          }
          else {
            // receive data
            FanOut<UserType>::impl->read();
          }
          for(auto &slave : FanOut<UserType>::slaves) {     // send out copies to slaves
            // do not send copy if no data is expected (e.g. trigger)
            if(slave->getNumberOfSamples() != 0) {
              slave->accessChannel(0) = FanOut<UserType>::impl->accessChannel(0);
            }
            slave->write();
          }
        }
      }
      
    protected:

      bool hasExternalTrigger{false};
      boost::shared_ptr<mtca4u::TransferElement> externalTrigger;

      /** Thread handling the synchronisation, if needed */
      boost::thread _thread;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_THREADED_FAN_OUT_H */

