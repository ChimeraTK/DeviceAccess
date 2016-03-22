/*
 * RegisterAccessor2D.h
 *
 *  Created on: Feb 3, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_REGISTER_ACCESSOR_2D_H
#define MTCA4U_REGISTER_ACCESSOR_2D_H

#include <boost/smart_ptr.hpp>

#include "NDRegisterAccessor.h"

namespace mtca4u {

  /** TODO add documentation
   */
  template<class UserType>
  class TwoDRegisterAccessor : public TransferElement {

    public:

      /** Do not use this constructor directly. Instead call Device::getRegisterAccessor2D(). */
      TwoDRegisterAccessor( boost::shared_ptr< NDRegisterAccessor<UserType> > _accessor )
      : _impl(_accessor)
      {}

      /** Placeholder constructer, to allow late initialisation of the accessor, e.g. in the open function.
       *  @attention Accessors created with this constructors will be dysfunctional, calling any member function
       *  will throw an exception (by the boost::shared_ptr)! */
      TwoDRegisterAccessor()
      {}

      /** Default destructor */
      ~TwoDRegisterAccessor() {}

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

      bool isReadOnly() const {
        return _impl->isReadOnly();
      }

      virtual std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() {
        return _impl->getHardwareAccessingElements();
      }

      virtual boost::shared_ptr<TransferElement> getHighLevelImplElement() {
        return boost::static_pointer_cast<TransferElement>(_impl);
      }

      virtual void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) {
        if(_impl->isSameRegister(newElement)) {
          _impl = boost::dynamic_pointer_cast< NDRegisterAccessor<UserType> >(newElement);
        }
        else {
          _impl->replaceTransferElement(newElement);
        }
      }

    protected:

      /** pointer to the implementation */
      boost::shared_ptr< NDRegisterAccessor<UserType> > _impl;

      virtual bool isSameRegister(const boost::shared_ptr<TransferElement const> &other) const {
        return _impl->isSameRegister(other);
      }

  };

} // namespace mtca4u

#endif /* MTCA4U_REGISTER_ACCESSOR_2D_H */
