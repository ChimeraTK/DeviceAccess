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
   * value and ignors any write operations.

   * If AccessMode::wait_for_new_data was set, TransferElement::read() will
   * return once with the initial value, and the block on the second call, waiting for new
   * data which obviously nerver arrives. A blocking call can be interrupted by
   * calling TransferElemtent::interrupt, which will throw a boost::thread_interrupted exception.
   *
   * For writing, it conceptually works like /dev/null. The data is intentionally dropped and
   * not considered "lost". Hence write() and writeNonBlocking() always return 'false' (no data
   * was lost), so it can also be connected to modules which retry sending data for fault recovery
   * until they succeed.
   */
  template<typename UserType>
  class ConstantAccessor : public ChimeraTK::NDRegisterAccessor<UserType> {
   public:
    ConstantAccessor(UserType value = 0, size_t length = 1, AccessModeFlags accessModeFlags = AccessModeFlags{})
    : ChimeraTK::NDRegisterAccessor<UserType>("UnnamedConstantAccessor", accessModeFlags), _value(length, value) {
      ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D.resize(1);
      ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D[0] = _value;

      if(TransferElement::_accessModeFlags.has(AccessMode::wait_for_new_data)) {
        // This implementation does not have a data transport queue, hence _readQueue
        // is not set up as a continuation. We directly make it a cppext::future_queue
        TransferElement::_readQueue = cppext::future_queue<void>(3); // minimum required length is 2
        // Push once into the queue for the initial value.
        TransferElement::_readQueue.push();
      }
    }

    void doReadTransferSynchronously() override {}

    void doPostRead(TransferType /*type*/, bool updateUserBuffer) override {
      // - updateUserBuffer is false for further calls to readLatest with wait_for_new_data.
      //   In this case the user buffer must not be touched.
      // - updateUserBuffer is true for all calls without wait_for_new_data. The user buffer must
      //   be overwritten.
      if(updateUserBuffer) {
        ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D[0] = _value;
        // It is OK to generate the version number just here since the read transfer is empty anyway.
        this->_versionNumber = {};
        this->_dataValidity = DataValidity::ok; // the constant is always valid by definiton
      }
    }

    bool doWriteTransfer(ChimeraTK::VersionNumber /*versionNumber*/ = {}) override { return false; }

    bool mayReplaceOther(const boost::shared_ptr<ChimeraTK::TransferElement const>&) const override { return false; }

    bool isReadOnly() const override { return false; }

    bool isReadable() const override { return true; }

    bool isWriteable() const override { return true; }

    std::vector<boost::shared_ptr<ChimeraTK::TransferElement>> getHardwareAccessingElements() override { return {}; }

    void replaceTransferElement(boost::shared_ptr<ChimeraTK::TransferElement>) override {}

    std::list<boost::shared_ptr<ChimeraTK::TransferElement>> getInternalElements() override { return {}; }

    void interrupt() override { TransferElement::interrupt_impl(this->_readQueue); }

   protected:
    std::vector<UserType> _value;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_CONSTANT_ACCESSOR_H */
