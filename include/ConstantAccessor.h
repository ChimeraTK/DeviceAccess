/*
 * ConstantAccessor.h
 *
 *  Created on: Jun 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_CONSTANT_ACCESSOR_H
#define CHIMERATK_CONSTANT_ACCESSOR_H

#include <ChimeraTK/NDRegisterAccessor.h>

namespace ChimeraTK {

  /** Implementation of the NDRegisterAccessor which delivers always the same
   * If AccessMode::wait_for_new_data was set, TransferElement::read() will
   * still block until new data has arrived.
   * value and ignors any write operations */
  template<typename UserType>
  class ConstantAccessor : public ChimeraTK::NDRegisterAccessor<UserType> {
   public:
    ConstantAccessor(UserType value = 0, size_t length = 1, AccessModeFlags accessModeFlags = AccessModeFlags{})
    : ChimeraTK::NDRegisterAccessor<UserType>("UnnamedConstantAccessor", accessModeFlags), _value(length, value) {
      ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D.resize(1);
      ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D[0] = _value;

      if(TransferElement::_accessModeFlags.has(AccessMode::wait_for_new_data)) {
        TransferElement::_readQueue = cppext::future_queue<void>(0);
        TransferElement::_readQueue.push();
      }
    }

    ~ConstantAccessor() {}

    void doReadTransferSynchronously() override {}

    void doPostRead(TransferType /*type*/, bool /* hasNewData */) override {
      ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D[0] = _value;
    }

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

   protected:
    std::vector<UserType> _value;

    bool firstRead{true};

    bool isInterrupted{false};
    boost::promise<void> promise;

    VersionNumber versionNumber{nullptr};
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_CONSTANT_ACCESSOR_H */
