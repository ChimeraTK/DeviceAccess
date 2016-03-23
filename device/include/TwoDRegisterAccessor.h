/*
 * TwoDRegisterAccessor.h
 *
 *  Created on: Feb 3, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_TWO_DREGISTER_ACCESSOR_H
#define MTCA4U_TWO_DREGISTER_ACCESSOR_H

#include <boost/smart_ptr.hpp>

#include "ForwardDeclarations.h"
#include "NDRegisterAccessor.h"

namespace mtca4u {

  /** TODO add documentation
   */
  template<class UserType>
  class TwoDRegisterAccessor {

    public:

      /** Do not use this constructor directly. Instead call Device::getTwoDRegisterAccessor(). */
      TwoDRegisterAccessor( boost::shared_ptr< NDRegisterAccessor<UserType> > _accessor )
      : _impl(_accessor)
      {}

      /** Placeholder constructer, to allow late initialisation of the accessor, e.g. in the open function.
       *  @attention Accessors created with this constructors will be dysfunctional, calling any member function
       *  will throw an exception (by the boost::shared_ptr)! */
      TwoDRegisterAccessor()
      {}

      /** Operator to access individual sequences/channels. */
      std::vector<UserType> & operator[](size_t channel) {
        return _impl->buffer_2D[channel];
      }

      /** Const operator to access individual sequences/channels. */
      const std::vector<UserType> & operator[](size_t channel) const {
        return _impl->buffer_2D[channel];
      }

      /** Read the data from the device, convert it and store in buffer. */
      void read() {
        _impl->read();
      }

      /** Convert data from the buffer and write to device. */
      void write() {
        _impl->write();
      }

      /** Return the number of sequences (=channels) */
      size_t getNumberOfDataSequences() const {
        return _impl->getNumberOfChannels();
      }

      /** Return the number of sequences (=channels) */
      size_t getNumberOfChannels() const {
        return _impl->getNumberOfChannels();
      }

      /** Return number of samples per sequence (=channel) */
      size_t getNumberOfSamples() const {
        return _impl->getNumberOfSamples();
      }

      /** Return if the register accessor allows only reading */
      bool isReadOnly() const {
        return _impl->isReadOnly();
      }

      /** Return if the accessor is properly initialised. It is initialised if it was constructed passing the pointer
       *  to an implementation (a NDRegisterAccessor), it is not initialised if it was constructed only using the
       *  placeholder constructor without arguments. */
      bool isInitialised() const {
        return _impl != NULL;
      }

    protected:

      /** pointer to the implementation */
      boost::shared_ptr< NDRegisterAccessor<UserType> > _impl;


      /** return the implementation, used for adding the accessor to a TransferGroup */
      boost::shared_ptr<TransferElement> getHighLevelImplElement() {
        return boost::static_pointer_cast<TransferElement>(_impl);
      }

      friend class TransferGroup;

  };

} // namespace mtca4u

#endif /* MTCA4U_TWO_DREGISTER_ACCESSOR_H */
