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

      ConstantAccessor(UserType value=0, size_t length=1)
      : _value(length, value)
      {
        mtca4u::NDRegisterAccessor<UserType>::buffer_2D.resize(1);
        mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0] = _value;
      }
      
      void doReadTransfer() override {
        if(firstRead) {
          firstRead = false;
          return;
        }
        // block forever
        boost::promise<void>().get_future().wait();
      }
            
      bool doReadTransferNonBlocking() override {
        if(firstRead) {
          firstRead = false;
          return true;
        }
        return false;
      }

      void postRead() override {
        mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0] = _value;
      }
      
      void write() override {
      }

      bool isSameRegister(const boost::shared_ptr<mtca4u::TransferElement const>&) const override {return false;}

      bool isReadOnly() const override {return false;}

      bool isReadable() const override {return true;}
      
      bool isWriteable() const override {return true;}

      std::vector< boost::shared_ptr<mtca4u::TransferElement> > getHardwareAccessingElements() override {return{};}

      void replaceTransferElement(boost::shared_ptr<mtca4u::TransferElement>) override {}
      
    protected:
      
      std::vector<UserType> _value;
      
      bool firstRead{true};

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_CONSTANT_ACCESSOR_H */
