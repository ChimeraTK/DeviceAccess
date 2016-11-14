/*
 * ConstantAccessor.h
 *
 *  Created on: Jun 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_CONSTANT_ACCESSOR_H
#define CHIMERATK_CONSTANT_ACCESSOR_H

#include <mtca4u/NDRegisterAccessor.h>

namespace ChimeraTK {

  /** Implementation of the NDRegisterAccessor which delivers always the same value and ignors any write operations */
  template<typename UserType>
  class ConstantAccessor : public mtca4u::NDRegisterAccessor<UserType> {

    public:

      /** Use this constructor if the FanOut should be a consuming implementation. */
      ConstantAccessor(UserType value=0, size_t length=1)
      : _value(length, value)
      {
        mtca4u::NDRegisterAccessor<UserType>::buffer_2D.resize(1);
        mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0] = _value;
      }
      
      void read() {
        mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0] = _value;
      }
      
      bool readNonBlocking() {
        mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0] = _value;
        return true;
      }
      
      void write() {
      }

      bool isSameRegister(const boost::shared_ptr<mtca4u::TransferElement const>&) const {return false;}

      bool isReadOnly() const {return false;}

      bool isReadable() const {return true;}
      
      bool isWriteable() const {return true;}

      std::vector< boost::shared_ptr<mtca4u::TransferElement> > getHardwareAccessingElements() {return{};}

      void replaceTransferElement(boost::shared_ptr<mtca4u::TransferElement>) {}
      
    protected:
      
      std::vector<UserType> _value;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_CONSTANT_ACCESSOR_H */
