/*
 * TwoDRegisterAccessor.h
 *
 *  Created on: Feb 3, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_TWO_DREGISTER_ACCESSOR_H
#define MTCA4U_TWO_DREGISTER_ACCESSOR_H

#include <boost/smart_ptr.hpp>

#include "NDRegisterAccessorBridge.h"

namespace mtca4u {

  /** TODO add documentation
   */
  template<class UserType>
  class TwoDRegisterAccessor : public NDRegisterAccessorBridge<UserType> {

    public:

      /** Do not use this constructor directly. Instead call Device::getTwoDRegisterAccessor(). */
      TwoDRegisterAccessor( boost::shared_ptr< NDRegisterAccessor<UserType> > _accessor )
      : NDRegisterAccessorBridge<UserType>(_accessor)
      {}

      /** Placeholder constructer, to allow late initialisation of the accessor, e.g. in the open function.
       *  @attention Accessors created with this constructors will be dysfunctional, calling any member function
       *  will throw an exception (by the boost::shared_ptr)! */
      TwoDRegisterAccessor()
      {}

      /** Operator to access individual sequences/channels. */
      std::vector<UserType> & operator[](size_t channel) {
        return NDRegisterAccessorBridge<UserType>::_impl->accessChannel(channel);
      }

      /** Const operator to access individual sequences/channels. */
      const std::vector<UserType> & operator[](size_t channel) const {
        return NDRegisterAccessorBridge<UserType>::_impl->accessChannel(channel);
      }

      /** Read the data from the device, convert it and store in buffer. */
      void read() {
        NDRegisterAccessorBridge<UserType>::_impl->read();
      }

      bool readNonBlocking(){
	return NDRegisterAccessorBridge<UserType>::_impl->readNonBlocking();
      }

     /** Convert data from the buffer and write to device. */
      void write() {
        NDRegisterAccessorBridge<UserType>::_impl->write();
      }

      /** Return the number of channels (formerly called sequences) */
      size_t getNChannels() const {
        return NDRegisterAccessorBridge<UserType>::_impl->getNumberOfChannels();
      }

      /** Return number of elements/samples per channel */
      size_t getNElementsPerChannel() const {
        return NDRegisterAccessorBridge<UserType>::_impl->getNumberOfSamples();
      }

      /** Return if the register accessor allows only reading */
      bool isReadOnly() const {
        return NDRegisterAccessorBridge<UserType>::_impl->isReadOnly();
      }

      bool isReadable() const {
        return NDRegisterAccessorBridge<UserType>::_impl->isReadable();
      }

      bool isWriteable() const {
        return NDRegisterAccessorBridge<UserType>::_impl->isWriteable();
      }

      /** Return if the accessor is properly initialised. It is initialised if it was constructed passing the pointer
       *  to an implementation (a NDRegisterAccessor), it is not initialised if it was constructed only using the
       *  placeholder constructor without arguments. */
      bool isInitialised() const {
        return NDRegisterAccessorBridge<UserType>::_impl != NULL;
      }

      /** DEPRECATED DO NOT USE
       *
       *  \deprecated
       *  This function is deprecated. Use getNChannels() instead! */
      size_t getNumberOfDataSequences() const {
        return NDRegisterAccessorBridge<UserType>::_impl->getNumberOfChannels();
      }

      /** DEPRECATED DO NOT USE
       *
       *  \deprecated
       *  This function is deprecated. Use getNChannels() instead! */
      size_t getNumberOfChannels() const {
        return NDRegisterAccessorBridge<UserType>::_impl->getNumberOfChannels();
      }

      /** DEPRECATED DO NOT USE
       *
       *  \deprecated
       *  This function is deprecated. Use getNElementsPerChannel() instead! */
      size_t getNumberOfSamples() const {
        return NDRegisterAccessorBridge<UserType>::_impl->getNumberOfSamples();
      }

      friend class TransferGroup;

  };

} // namespace mtca4u

#endif /* MTCA4U_TWO_DREGISTER_ACCESSOR_H */
