/*
 * SyncNDRegisterAccessor.h
 *
 *  Created on: Nov 22, 2017
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_SYNC_N_D_REGISTER_ACCESSOR_H
#define CHIMERA_TK_SYNC_N_D_REGISTER_ACCESSOR_H

#include <boost/make_shared.hpp>

#include "ForwardDeclarations.h"
#include "TransferElement.h"
#include "FixedPointConverter.h"
#include "DeviceException.h"
#include "NDRegisterAccessor.h"

namespace ChimeraTK {

  /** NDRegisterAccessor for backends with only synchronous transfers (so readAsync() must be implemented with a
   *  thread). This class just provides a default implementation of readAsync() for those backends. */
  template<typename UserType>
  class SyncNDRegisterAccessor : public NDRegisterAccessor<UserType> {

    public:
      SyncNDRegisterAccessor(std::string const &name,
                             std::string const &unit = std::string(TransferElement::unitNotSet),
                             std::string const &description = std::string())
      : NDRegisterAccessor<UserType>(name, unit, description)
      {}

      ~SyncNDRegisterAccessor() {
        // This is a requirement to all implementations: call shutdown() in the destructor!
        assert(shutdownCalled);
      }

      /**
       * All implementations must call this function in their destructor. Also, implementations must call it in their
       * constructors before throwing an exception (hint: put catch-all block around the entired constructor, call
       * shutdown() there and then rethrow the exception).
       *
       * Implementation note: This function call is necessary to ensure that a potentially still-running thread
       * launched in readAsync() is properly terminated before destroying the accessor object. Since this thread
       * accesses virtual functions like doReadTransfer(), the full accessor object must still be alive, thus shutting
       * down the thread in the base class destructor is too late. Technically, implementations overriding readAsync()
       * would not need to call this function, but to make sure all implementations which do not override readAsync()
       * actually call it, the function call is enforced for all implementations in the destructor with an assert.
       */
      void shutdown() {
        if(readAsyncThread.joinable()) {
          readAsyncThread.interrupt();
          readAsyncThread.join();
        }
        shutdownCalled = true;
      }

      TransferFuture doReadTransferAsync() {
        // create future_queue if not already created and continue it to enusre postRead is called (in the user thread,
        // so we use the deferred launch policy)
        if(!futureCreated) {
          notifications = cppext::future_queue<void>(2);
          activeFuture = TransferFuture(notifications, this);
          futureCreated = true;
        }

        // launch doReadTransfer in separate thread
        readAsyncThread = boost::thread(
          [this] {
            try {
              this->doReadTransfer();
            }
            catch(...) {
              this->notifications.push_exception(std::current_exception());
              throw;
            }
            this->notifications.push();
          }
        );

        // return the TransferFuture
        return activeFuture;
      }

    private:

      /// Thread which might be launched in readAsync()
      boost::thread readAsyncThread;

      /// future_queue used to notify the TransferFuture about completed transfers
      cppext::future_queue<void> notifications;

      /// Flag whether TransferFuture has been created
      bool futureCreated{false};

      /// Flag whether shutdown() has been called or not
      bool shutdownCalled{false};

      using TransferElement::activeFuture;

  };

}

#endif /* CHIMERA_TK_SYNC_N_D_REGISTER_ACCESSOR_H */

