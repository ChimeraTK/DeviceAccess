// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AsyncDomain.h"
#include "DeviceBackend.h"
#include "NDRegisterAccessor.h"

#include <ChimeraTK/cppext/finally.hpp>
#include <ChimeraTK/cppext/future_queue.hpp>

namespace ChimeraTK {

  class AsyncAccessorManager;

  /** The AsyncNDRegisterAccessor implements a data transport queue with typed data
   *  as continuation of the void queue in TransferElement. This allows to
   *  receive the content of the buffer_2D, the version number and the data validity flag.
   *  The implementation is complete. The interrupt handling thread in the backend implementation
   *  can write to the queues through the member functions.
   */
  template<typename UserType>
  class AsyncNDRegisterAccessor : public NDRegisterAccessor<UserType> {
    static constexpr size_t _queueSize{3};

   public:
    /** In addition to the arguments of the NDRegisterAccessor constructor, you need
     *  an AsyncAccessorManager where you can unsubscribe. As the AsyncAccessorManager is
     *  the factory for AsyncNDRegisterAccessor, this is only an implementation detail.
     */
    AsyncNDRegisterAccessor(boost::shared_ptr<DeviceBackend> backend, boost::shared_ptr<AsyncAccessorManager> manager,
        boost::shared_ptr<AsyncDomain> asyncDomain, std::string const& name, size_t nChannels, size_t nElements,
        AccessModeFlags accessModeFlags, std::string const& unit = std::string(TransferElement::unitNotSet),
        std::string const& description = std::string());

    ~AsyncNDRegisterAccessor() override;

    /** Pushes the exception to the queue. Must only be called from within deactivateAsyncAndExecute()
     *  to make sure it does not conflict with a re-enabling, which might violate the order guarantee in the spec.
     */
    void sendException(std::exception_ptr& e) { _dataTransportQueue.push_overwrite_exception(e); }

    /** You can only send destructively. If you want to keep a copy you have to make one yourself.
     *  This is more efficient that having one extra buffer within each AsyncNDRegisterAccessor.
     */
    void sendDestructively(typename NDRegisterAccessor<UserType>::Buffer& data);

    ////////////////////////////////////////////////////
    // implementation of inherited, virtual functions //
    ////////////////////////////////////////////////////

    void doReadTransferSynchronously() override {
      // This code should never be executed because the constructor checks that wait_for_new_data is set.
      assert(false);
    }

    bool doWriteTransfer([[maybe_unused]] ChimeraTK::VersionNumber versionNumber) override {
      assert(false);
      return false;
    }

    bool doWriteTransferDestructively([[maybe_unused]] ChimeraTK::VersionNumber versionNumber) override {
      assert(false);
      return false;
    }

    void doPreWrite([[maybe_unused]] TransferType type, [[maybe_unused]] VersionNumber versionNumber) override {
      throw ChimeraTK::logic_error("Writing is not supported for " + this->getName());
    }

    void doPreRead([[maybe_unused]] TransferType type) override {
      if(!_backend->isOpen()) throw ChimeraTK::logic_error("Device not opened.");
      // Pre-read conceptually does nothing in asynchronous reads
    }

    // Don't ask my why in template code the [[maybe_unused]] must be here, but gives a warning when put to the
    // implementation.
    void doPostRead([[maybe_unused]] TransferType type, bool updateDataBuffer) override;

    [[nodiscard]] bool isReadOnly() const override {
      return !isWriteable(); // as the accessor is always readable, isReadOnly() is equivalent to !isWriteable()
    }
    [[nodiscard]] bool isReadable() const override { return true; }
    [[nodiscard]] bool isWriteable() const override { return false; }

    void setExceptionBackend(boost::shared_ptr<DeviceBackend> exceptionBackend) override {
      this->_exceptionBackend = exceptionBackend;
    }

    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override {
      return {boost::enable_shared_from_this<TransferElement>::shared_from_this()};
    }
    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override { return {}; }

    void replaceTransferElement(boost::shared_ptr<TransferElement> /*newElement*/) override {} // LCOV_EXCL_LINE

    void interrupt() override { this->interrupt_impl(this->_dataTransportQueue); }

   protected:
    boost::shared_ptr<DeviceBackend> _backend;
    boost::shared_ptr<AsyncAccessorManager> _accessorManager;
    boost::shared_ptr<AsyncDomain> _asyncDomain;
    using typename NDRegisterAccessor<UserType>::Buffer;
    using NDRegisterAccessor<UserType>::buffer_2D;
    Buffer _receiveBuffer;

    cppext::future_queue<Buffer, cppext::SWAP_DATA> _dataTransportQueue{_queueSize};
  };

  /**********************************************************************************************************/
  /* Implementations are in the .cc file. We use the trick to declare for all known types here,             */
  /* and instantiate all of them in the .cc file. This is necessary because we cannot include               */
  /* AsyncAccessorManager.h to avoid a circular dependency, but need it for the implementation.             */
  /**********************************************************************************************************/
  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(AsyncNDRegisterAccessor);

} // namespace ChimeraTK
