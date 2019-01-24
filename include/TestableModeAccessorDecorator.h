/*
 * TestableModeAccessorDecorator.h
 *
 *  Created on: Feb 17, 2017
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_TEST_DECORATOR_REGISTER_ACCCESSOR
#define CHIMERATK_TEST_DECORATOR_REGISTER_ACCCESSOR

#include <ChimeraTK/NDRegisterAccessorDecorator.h>

#include "Application.h"
#include "FeedingFanOut.h"

namespace ChimeraTK {

  /** Decorator of the NDRegisterAccessor which facilitates tests of the application */
  template<typename UserType>
  class TestableModeAccessorDecorator : public ChimeraTK::NDRegisterAccessorDecorator<UserType> {
    public:
      TestableModeAccessorDecorator(boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> accessor,
                                    bool handleRead, bool handleWrite)
      : ChimeraTK::NDRegisterAccessorDecorator<UserType>(accessor),
        _handleRead(handleRead),
        _handleWrite(handleWrite)
      {

        // obtain variableId of target accessor
        variableId = Application::getInstance().idMap[this->_id];
        assert(variableId != 0);

        // if receiving end, register for testable mode (stall detection)
        if(this->isReadable()) {
          Application::getInstance().testableMode_processVars[variableId] = accessor;
        }

        // if this decorating a bidirectional process variable, set the valueRejectCallback
        auto bidir = boost::dynamic_pointer_cast<BidirectionalProcessArray<UserType>>(accessor);
        if(bidir) {
          bidir->setValueRejectCallback([this]{
            decrementCounter();
          });
        }

      }

      bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber={}) override {
        if(!_handleWrite) return _target->doWriteTransfer(versionNumber);

        bool dataLost = false;
        if(!Application::testableModeTestLock()) {
          // may happen if first write in thread is done before first blocking read
          Application::testableModeLock("write "+this->getName());
        }
        dataLost = _target->doWriteTransfer(versionNumber);
        if(!dataLost) {
          ++Application::getInstance().testableMode_counter;
          ++Application::getInstance().testableMode_perVarCounter[variableId];
          if(Application::getInstance().enableDebugTestableMode) {
            std::cout << "TestableModeAccessorDecorator::write[name='"<<this->getName()<<"', id="<<variableId<<"]: testableMode_counter "
                         "increased, now at value " << Application::getInstance().testableMode_counter << std::endl;
          }
        }
        else {
          if(Application::getInstance().enableDebugTestableMode) {
            std::cout << "TestableModeAccessorDecorator::write[name='"<<this->getName()<<"', id="<<variableId<<"]: testableMode_counter not "
                        "increased due to lost data" << std::endl;
          }
        }
        return dataLost;
      }

      void doReadTransfer() override {
        if(_handleRead) releaseLock();
        _target->doReadTransfer();
      }

      /** Release the testableModeLock */
      void releaseLock() {
        if(Application::testableModeTestLock()) Application::testableModeUnlock("doReadTransfer "+this->getName());
      }

      /** Implement callback called by TransferFuture before it blocks in wait() */
      void transferFutureWaitCallback() override {
        releaseLock();
      }

      /** Obtain the testableModeLock if not owned yet, and decrement the counter. */
      void obtainLockAndDecrementCounter() {
        if(!Application::testableModeTestLock()) Application::testableModeLock("doReadTransfer "+this->getName());
        decrementCounter();
      }

      /** Obtain the testableModeLock if not owned yet, and decrement the counter. */
      void decrementCounter() {
        if(Application::getInstance().testableMode_perVarCounter[variableId] > 0) {
          assert(Application::getInstance().testableMode_counter > 0);
          --Application::getInstance().testableMode_counter;
          --Application::getInstance().testableMode_perVarCounter[variableId];
          if(Application::getInstance().enableDebugTestableMode) {
            std::cout << "TestableModeAccessorDecorator[name='"<<this->getName()<<"', id="<<variableId<<"]: testableMode_counter "
                        "decreased, now at value " << Application::getInstance().testableMode_counter << " / " <<
                        Application::getInstance().testableMode_perVarCounter[variableId] << std::endl;
          }
        }
        else {
          if(Application::getInstance().enableDebugTestableMode) {
            std::cout << "TestableModeAccessorDecorator[name='"<<this->getName()<<"', id="<<variableId<<"]: testableMode_counter "
                        "NOT decreased, was already at value " << Application::getInstance().testableMode_counter << " / " <<
                        Application::getInstance().testableMode_perVarCounter[variableId] << std::endl;
          }
        }
      }

      bool doReadTransferNonBlocking() override {
        bool newData = _target->doReadTransferNonBlocking();
        if(!newData) return false;
        return true;
      }

      bool doReadTransferLatest() override {
        bool newData = _target->doReadTransferLatest();
        if(!newData) return false;

        // the queue has been emptied, so make sure that the testableMode_counter reflects this
        // we only reduce the counter to 1, since it will be decremented in postRead().
        if(_handleRead) {
          auto &app = Application::getInstance();
          assert(Application::testableModeTestLock());
          if(app.testableMode_perVarCounter[variableId] > 1) {
            app.testableMode_counter -= app.testableMode_perVarCounter[variableId] - 1;
            app.testableMode_perVarCounter[variableId] = 1;
          }
        }
        return true;
      }

      void doPostRead() override {
        if(_handleRead) obtainLockAndDecrementCounter();
        ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPostRead();
      }

    protected:

      using ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D;
      using ChimeraTK::NDRegisterAccessorDecorator<UserType>::_target;

      size_t variableId;

      bool _handleRead, _handleWrite;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_TEST_DECORATOR_REGISTER_ACCCESSOR */
