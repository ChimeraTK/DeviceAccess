/*
 * ThreadedFanOut.h
 *
 *  Created on: Jun 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_THREADED_FAN_OUT_H
#define CHIMERATK_THREADED_FAN_OUT_H

#include <mtca4u/NDRegisterAccessor.h>

#include "Application.h"
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
      

      ~ThreadedFanOut() {
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

      /** Synchronise feeder and the consumers. This function is executed in the separate thread. */
      void run() {
        Application::getInstance().testableModeThreadName() = "ThreadedFanOut "+FanOut<UserType>::impl->getName();
        Application::testableModeLock("start");
        while(true) {
          // receive data
          boost::this_thread::interruption_point();
          FanOut<UserType>::impl->read();
          boost::this_thread::interruption_point();
          // send out copies to slaves
          for(auto &slave : FanOut<UserType>::slaves) {
            // do not send copy if no data is expected (e.g. trigger)
            if(slave->getNumberOfSamples() != 0) {
              slave->accessChannel(0) = FanOut<UserType>::impl->accessChannel(0);
            }
            slave->write();
          }
        }
      }
      
    protected:

      /** Thread handling the synchronisation, if needed */
      boost::thread _thread;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_THREADED_FAN_OUT_H */

