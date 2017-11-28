/*
 * TestDecoratorRegisterAccessor.h
 *
 *  Created on: Feb 17, 2017
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_TEST_DECORATOR_REGISTER_ACCCESSOR
#define CHIMERATK_TEST_DECORATOR_REGISTER_ACCCESSOR

#include <mtca4u/NDRegisterAccessorDecorator.h>

#include "Application.h"

namespace ChimeraTK {

  /** Decorator of the NDRegisterAccessor which facilitates tests of the application */
  template<typename UserType>
  class TestDecoratorRegisterAccessor : public mtca4u::NDRegisterAccessorDecorator<UserType> {
    public:
      TestDecoratorRegisterAccessor(boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> accessor)
      : mtca4u::NDRegisterAccessorDecorator<UserType>(accessor)
      {

        // obtain variableId of target accessor
        variableId = Application::getInstance().idMap[this->_id];
        assert(variableId != 0);

        // if receiving end, register for testable mode (stall detection)
        if(this->isReadable()) {
          Application::getInstance().testableMode_processVars[variableId] = accessor;
        }
      }

      bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber={}) override {
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
            std::cout << "TestDecoratorRegisterAccessor::write[name='"<<this->getName()<<"', id="<<variableId<<"]: testableMode_counter "
                         "increased, now at value " << Application::getInstance().testableMode_counter << std::endl;
          }
        }
        else {
          if(Application::getInstance().enableDebugTestableMode) {
            std::cout << "TestDecoratorRegisterAccessor::write[name='"<<this->getName()<<"', id="<<variableId<<"]: testableMode_counter not "
                        "increased due to lost data" << std::endl;
          }
        }
        return dataLost;
      }

      void doReadTransfer() override {
        releaseLock();
        _target->doReadTransfer();
      }

      /** Release the testableModeLock */
      void releaseLock() {
        try {
          Application::testableModeUnlock("doReadTransfer "+this->getName());
        }
        catch(std::system_error &e) {   // ignore operation not permitted errors, since they happen the first time (lock not yet owned)
          if(e.code() != std::errc::operation_not_permitted) throw e;
        }
      }

      /** Implement callback called by TransferFuture before it blocks in wait() */
      void transferFutureWaitCallback() override {
        releaseLock();
      }

      /** Obtain the testableModeLock if not owned yet, and decrement the counter. */
      void obtainLockAndDecrementCounter() {
        if(!Application::testableModeTestLock()) Application::testableModeLock("doReadTransfer "+this->getName());
        if(Application::getInstance().testableMode_perVarCounter[variableId] > 0) {
          assert(Application::getInstance().testableMode_counter > 0);
          --Application::getInstance().testableMode_counter;
          --Application::getInstance().testableMode_perVarCounter[variableId];
          if(Application::getInstance().enableDebugTestableMode) {
            std::cout << "TestDecoratorRegisterAccessor[name='"<<this->getName()<<"', id="<<variableId<<"]: testableMode_counter "
                        "decreased, now at value " << Application::getInstance().testableMode_counter << " / " <<
                        Application::getInstance().testableMode_perVarCounter[variableId] << std::endl;
          }
        }
        else {
          if(Application::getInstance().enableDebugTestableMode) {
            std::cout << "TestDecoratorRegisterAccessor[name='"<<this->getName()<<"', id="<<variableId<<"]: testableMode_counter "
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
        auto &app = Application::getInstance();
        assert(Application::testableModeTestLock());
        if(app.testableMode_perVarCounter[variableId] > 1) {
          app.testableMode_counter -= app.testableMode_perVarCounter[variableId] - 1;
          app.testableMode_perVarCounter[variableId] = 1;
        }
        return true;
      }

      void postRead() override {
        obtainLockAndDecrementCounter();
        mtca4u::NDRegisterAccessorDecorator<UserType>::postRead();
      }

    protected:

      using mtca4u::NDRegisterAccessor<UserType>::buffer_2D;
      using mtca4u::NDRegisterAccessorDecorator<UserType>::_target;

      size_t variableId;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_TEST_DECORATOR_REGISTER_ACCCESSOR */
