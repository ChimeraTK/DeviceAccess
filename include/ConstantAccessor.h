/*
 * ConstantAccessor.h
 *
 *  Created on: Jun 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_CONSTANT_ACCESSOR_H
#define CHIMERATK_CONSTANT_ACCESSOR_H

#include <mtca4u/SyncNDRegisterAccessor.h>

namespace ChimeraTK {

  /** Implementation of the NDRegisterAccessor which delivers always the same value and ignors any write operations */
  template<typename UserType>
  class ConstantAccessor : public mtca4u::SyncNDRegisterAccessor<UserType> {

    public:

      ConstantAccessor(UserType value=0, size_t length=1)
        : mtca4u::SyncNDRegisterAccessor<UserType>("UnnamedConstantAccessor"), _value(length, value)
      {
        try {
          mtca4u::NDRegisterAccessor<UserType>::buffer_2D.resize(1);
          mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0] = _value;
        }
        catch(...) {
          this->shutdown();
          throw;
        }
      }

      ~ConstantAccessor() {
        this->shutdown();
      }

      void doReadTransfer() override {
        if(firstRead) {
          firstRead = false;
          return;
        }
        // block forever
        promise.get_future().wait();
        // if we get here, interrupt() has been called
        throw boost::thread_interrupted();
      }

      bool doReadTransferNonBlocking() override {
        if(firstRead) {
          firstRead = false;
          return true;
        }
        return false;
      }

      bool doReadTransferLatest() override {
        return doReadTransferNonBlocking();
      }

      void doPostRead() override {
        mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0] = _value;
      }

      bool doWriteTransfer(ChimeraTK::VersionNumber /*versionNumber*/={}) override {
        return true;
      }

      bool mayReplaceOther(const boost::shared_ptr<mtca4u::TransferElement const>&) const override {
        return false;   /// @todo implement properly?
      }

      bool isReadOnly() const override {return false;}

      bool isReadable() const override {return true;}

      bool isWriteable() const override {return true;}

      std::vector< boost::shared_ptr<mtca4u::TransferElement> > getHardwareAccessingElements() override {return{};}

      void replaceTransferElement(boost::shared_ptr<mtca4u::TransferElement>) override {}

      std::list<boost::shared_ptr<mtca4u::TransferElement> > getInternalElements() override {return {};}

      AccessModeFlags getAccessModeFlags() const override { return {}; }

      void interrupt() override { promise.set_value(); }

    protected:

      std::vector<UserType> _value;

      bool firstRead{true};

      boost::promise<void> promise;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_CONSTANT_ACCESSOR_H */
