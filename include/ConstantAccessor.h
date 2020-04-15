/*
 * ConstantAccessor.h
 *
 *  Created on: Jun 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_CONSTANT_ACCESSOR_H
#define CHIMERATK_CONSTANT_ACCESSOR_H

#include <ChimeraTK/SyncNDRegisterAccessor.h>

namespace ChimeraTK {

  /** Implementation of the NDRegisterAccessor which delivers always the same
   * value and ignors any write operations */
  template<typename UserType>
  class ConstantAccessor : public ChimeraTK::SyncNDRegisterAccessor<UserType> {
   public:
    ConstantAccessor(UserType value = 0, size_t length = 1)
    : ChimeraTK::SyncNDRegisterAccessor<UserType>("UnnamedConstantAccessor"), _value(length, value) {
      try {
        ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D.resize(1);
        ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D[0] = _value;
      }
      catch(...) {
        this->shutdown();
        throw;
      }
    }

    ~ConstantAccessor() { this->shutdown(); }

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

    bool doReadTransferLatest() override { return doReadTransferNonBlocking(); }

    void doPostRead(TransferType /*type*/, bool /* hasNewData */) override { ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D[0] = _value; }

    bool doWriteTransfer(ChimeraTK::VersionNumber /*versionNumber*/ = {}) override { return true; }

    bool mayReplaceOther(const boost::shared_ptr<ChimeraTK::TransferElement const>&) const override {
      return false; /// @todo implement properly?
    }

    bool isReadOnly() const override { return false; }

    bool isReadable() const override { return true; }

    bool isWriteable() const override { return true; }

    std::vector<boost::shared_ptr<ChimeraTK::TransferElement>> getHardwareAccessingElements() override { return {}; }

    void replaceTransferElement(boost::shared_ptr<ChimeraTK::TransferElement>) override {}

    std::list<boost::shared_ptr<ChimeraTK::TransferElement>> getInternalElements() override { return {}; }

    AccessModeFlags getAccessModeFlags() const override { return {}; }

    void interrupt() override {
      if(isInterrupted) return;
      promise.set_value();
      isInterrupted = true;
    }

    VersionNumber getVersionNumber() const override { return versionNumber; }

   protected:
    std::vector<UserType> _value;

    bool firstRead{true};

    bool isInterrupted{false};
    boost::promise<void> promise;

    VersionNumber versionNumber{nullptr};
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_CONSTANT_ACCESSOR_H */
