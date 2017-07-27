/*
 * NDRegisterAccessor.h - N-dimensional register acceesor
 *
 *  Created on: Mar 21, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_N_D_REGISTER_ACCESSOR_H
#define MTCA4U_N_D_REGISTER_ACCESSOR_H

#include <boost/make_shared.hpp>

#include "ForwardDeclarations.h"
#include "TransferElement.h"
#include "FixedPointConverter.h"
#include "DeviceException.h"
#include "ExperimentalFeatures.h"

namespace mtca4u {
  
  /** N-dimensional register accessor. Base class for all register accessor implementations. The user frontend classes
   *  BufferingRegisterAccessor and TwoDRegisterAccessor are using implementations based on this class to perform
   *  the actual IO. */
  template<typename UserType>
  class NDRegisterAccessor : public TransferElement {

    public:
      /** Creates an NDRegisterAccessor with the specified name (passed on to the 
       *  transfer element). */
      NDRegisterAccessor(std::string const &name = std::string(),
                         std::string const &unit = std::string(TransferElement::unitNotSet),
                         std::string const &description = std::string())
      : TransferElement(name, unit, description) {}
        
      /** A virtual base class needs a virtual destructor */
      virtual ~NDRegisterAccessor() {
        if(readAsyncThread.joinable()) {
          readAsyncThread.interrupt();
          readAsyncThread.join();
        }
      }

      /** Get or set register accessor's buffer content (1D version).
       *  @attention No bounds checking is performed, use getNumberOfSamples() to obtain the number of elements in
       *  the register. */
      UserType& accessData(unsigned int sample) {
        return buffer_2D[0][sample];
      }

      /** Get or set register accessor's buffer content (2D version).
       *  @attention No bounds checking is performed, use getNumberOfChannels() and getNumberOfSamples() to obtain the
       *   number of channels and samples in the register. */
      UserType& accessData(unsigned int channel, unsigned int sample) {
        return buffer_2D[channel][sample];
      }

      /** Get or set register accessor's channel vector.
       *  @attention No bounds checking is performed, use getNumberOfChannels() to obtain the number of elements in
       *  the register. */
      std::vector<UserType>& accessChannel(unsigned int channel) {
        return buffer_2D[channel];
      }

      /** Return number of elements per channel */
      unsigned int getNumberOfSamples() const {
        return buffer_2D[0].size();
      }

      /** Return number of channels */
      unsigned int getNumberOfChannels() const {
        return buffer_2D.size();
      }

      virtual const std::type_info& getValueType() const override{
        return typeid(UserType);
      }

      TransferFuture& readAsync() override {
        ChimeraTK::ExperimentalFeatures::check("asynchronous read");
        if(hasActiveFuture) return activeFuture;  // the last future given out by this fuction is still active
        
        // create promise future pair and launch doReadTransfer in separate thread
        readAsyncPromise = TransferFuture::PromiseType();
        auto boostFuture = readAsyncPromise.get_future().share();
        readAsyncThread = boost::thread(
          [this] {
            doReadTransfer();
            transferFutureData._versionNumber = VersionNumber();
            readAsyncPromise.set_value(&transferFutureData);
          }
        );
        
        // form TransferFuture, store it for later re-used and return it
        activeFuture = TransferFuture(boostFuture, static_cast<TransferElement*>(this));
        hasActiveFuture = true;
        return activeFuture;
      }

      /** DEPRECATED DO NOT USE! Instead make a call to readNonBlocking() and check the return value.
       *  \deprecated This function is deprecated, remove it at some point!
       * 
       *  This function was deprecated since it cannot be implemented for lockfree implementations (like the
       *  ControlSystemAdapter's ProcessVariable).
       *  
       *  Return number of waiting data elements in the queue (or buffer). Use when the accessor was obtained with
       *  AccessMode::wait_for_new_data to obtain the amount of data waiting for retrieval in this accessor. If the
       *  returned value is 0, the call to read() will block until new data has arrived. If the returned value is > 0,
       *  it is guaranteed that the next call to read() will not block. If the accessor was obtained without the
       *  AccessMode::wait_for_new_data flag, this function will always return 1. */
      virtual unsigned int getNInputQueueElements() const {
        return 1;
      }

      /** DO NOT USE. FOR BACKWARDS COMPATIBILITY ONLY.
       *
       *  \deprecated This function is for backwards compatibility with the deprecated RegisterAccessor only.
       *  Return the fixed point converter used to convert the raw data from the device to the type T. If no conversion
       *  by the fixed point converter is required, this function will throw an exception. */
      virtual FixedPointConverter getFixedPointConverter() const  {
        throw DeviceException("Not implemented", DeviceException::NOT_IMPLEMENTED);
      }

    protected:

      /** Buffer of converted data elements. The buffer is always two dimensional. If a register with a single
       *  dimension should be accessed, the outer vector has only a single element. For a scalar register, only a
       *  single element is present in total (buffer_2D[0][0]). This has a negligible performance impact when
       *  optimisations are enabled, but allows a coherent interface for all accessors independent of their dimension.
       *
       *  Implementation note: The buffer must be created with the right number of elements in the constructor! */
      std::vector< std::vector<UserType> > buffer_2D;

      /// the compatibility layers need access to the buffer_2D
      friend class MultiplexedDataAccessor<UserType>;
      friend class RegisterAccessor;
      
      /// Thread which might be launched in readAsync()
      boost::thread readAsyncThread;
      
      /// Promise used in readAsync()
      TransferFuture::PromiseType readAsyncPromise;
      
      /// Data transferred in the TransferFuture, used by the default implementation of readAsync()
      TransferFuture::Data transferFutureData{{}};

  };

}

#endif /* MTCA4U_N_D_REGISTER_ACCESSOR_H */
