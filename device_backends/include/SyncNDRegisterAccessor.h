/*
 * SyncNDRegisterAccessor.h
 *
 *  Created on: Nov 22, 2017
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_SYNC_N_D_REGISTER_ACCESSOR_H
#define MTCA4U_SYNC_N_D_REGISTER_ACCESSOR_H

#include <boost/make_shared.hpp>

#include "ForwardDeclarations.h"
#include "TransferElement.h"
#include "FixedPointConverter.h"
#include "DeviceException.h"
#include "ExperimentalFeatures.h"
#include "NDRegisterAccessor.h"

namespace mtca4u {

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

      TransferFuture& readAsync() final {
        ChimeraTK::ExperimentalFeatures::check("asynchronous read");
        if(TransferElement::hasActiveFuture) return activeFuture;  // the last future given out by this fuction is still active

        this->preRead();
        // create promise future pair and launch doReadTransfer in separate thread
        readAsyncPromise = TransferFuture::PromiseType();
        auto boostFuture = readAsyncPromise.get_future().share();
        readAsyncThread = boost::thread(
          [this] {
            this->doReadTransfer();
            // Do not call postRead() here. This thread is not allowed to touch the user space buffers.
            // postRead() will be called in the user thread in TransferFuture::wait().
            transferFutureData._versionNumber = VersionNumber();
            readAsyncPromise.set_value(&transferFutureData);
          }
        );

        // form TransferFuture, store it for later re-used and return it
        activeFuture.reset(boostFuture, static_cast<TransferElement*>(this));
        TransferElement::hasActiveFuture = true;
        return activeFuture;
      }

    private:

      /// Thread which might be launched in readAsync()
      boost::thread readAsyncThread;

      /// Promise used in readAsync()
      TransferFuture::PromiseType readAsyncPromise;

      /// last future returned by readAsync()
      TransferFuture activeFuture;

      /// Data transferred in the TransferFuture
      TransferFuture::Data transferFutureData{{}};

      /// Flag whether shutdown() has been called or not
      bool shutdownCalled{false};

  };

}

#endif /* MTCA4U_SYNC_N_D_REGISTER_ACCESSOR_H */

