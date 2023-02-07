// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

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
   *  can write to the queues through the member functions, and activate and deactivate the accessor.
   */
  template<typename UserType>
  class AsyncNDRegisterAccessor : public NDRegisterAccessor<UserType> {
    static constexpr size_t _queueSize{3};

   public:
    /** In addition to the arguments of the NDRegisterAccessor constructor, you need
     *  an AsyncAccessorManager where you can unsubscribe. As the AsyncAccessorManager is
     *  the factory for AsyncNDRegisterAccessor, this is only an implementation detail.
     */
    AsyncNDRegisterAccessor(const boost::shared_ptr<DeviceBackend>& backend,
        const boost::shared_ptr<AsyncAccessorManager>& manager, std::string const& name, size_t nChannels,
        size_t nElements, AccessModeFlags accessModeFlags,
        std::string const& unit = std::string(TransferElement::unitNotSet),
        std::string const& description = std::string());

    ~AsyncNDRegisterAccessor() override;

    /** Pushes the exception to the queue and marks the accessor as inactive, so nothing
     *  is pushed until re-activation.
     */
    void sendException(std::exception_ptr& e) {
      if(_isActive) {
        _dataTransportQueue.push_overwrite_exception(e);
      }
      _isActive = false;
    }

    /** You can only send destructively. If you want to keep a copy you have to make one yourself.
     *  This is more efficient that having one extra buffer within each AsyncNDRegisterAccessor.
     */
    void sendDestructively(typename NDRegisterAccessor<UserType>::Buffer& data) {
      if(!_isActive) { // || _hasException) {
        return;
      }

      _dataTransportQueue.push_overwrite(std::move(data));
    }

    /** Activate the accessor and send the initial value.
     */
    void activate(typename NDRegisterAccessor<UserType>::Buffer& initialValue) {
      //_hasException = false;
      _isActive = true;
      sendDestructively(initialValue);
    }

    /** Needs to be called in case a device is closed.
     */
    void deactivate() { _isActive = false; }

    /** Set an accessor if the register is writeable. As the asynchronous mechanism is generic and does not
     *  access the hardware, an implementation which can write is added here.
     */
    void setWriteAccessor(boost::shared_ptr<NDRegisterAccessor<UserType>> const& writeAccessor) {
      assert(writeAccessor->isWriteable());
      _writeAccessor = writeAccessor;
    }

    ////////////////////////////////////////////////////
    // implementation of inherited, virtual functions //
    ////////////////////////////////////////////////////

    void doReadTransferSynchronously() override {
      // This code should never be executed because the constructor checks that wait_for_new_data is set.
      assert(false);
    }

    bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber) override {
      return _writeAccessor->writeTransfer(versionNumber);
    }

    bool doWriteTransferDestructively(ChimeraTK::VersionNumber versionNumber) override {
      return _writeAccessor->writeTransferDestructively(versionNumber);
    }

    void doPreWrite(TransferType type, VersionNumber versionNumber) override {
      if(!_writeAccessor) {
        throw ChimeraTK::logic_error("Writing is not supported for " + this->getName());
      }
      // The following code is taken from the NDRegisterAccessorDecorator:
      for(size_t i = 0; i < _writeAccessor->getNumberOfChannels(); ++i) {
        buffer_2D[i].swap(_writeAccessor->accessChannel(i));
      }
      _writeAccessor->setDataValidity(this->_dataValidity);
      _writeAccessor->preWrite(type, versionNumber);
    }

    void doPreRead([[maybe_unused]] TransferType type) override {
      if(!_backend->isOpen()) throw ChimeraTK::logic_error("Device not opened.");
      // Pre-read conceptually does nothing in synchronous reads
    }

    void doPostWrite(TransferType type, VersionNumber versionNumber) override {
      // The following code is taken from the NDRegisterAccessorDecorator:
      // swap back buffers unconditionally (even if postWrite() throws) at the end of this function
      auto _ = cppext::finally([&] {
        for(size_t i = 0; i < _writeAccessor->getNumberOfChannels(); ++i) {
          buffer_2D[i].swap(_writeAccessor->accessChannel(i));
        }
      });
      _writeAccessor->setActiveException(this->_activeException);
      _writeAccessor->postWrite(type, versionNumber);
    }

    // Don't ask my why in template code the [[maybe_unused]] must be here, but gives a warning when put to the
    // implementation.
    void doPostRead([[maybe_unused]] TransferType type, bool updateDataBuffer) override;

    [[nodiscard]] bool isReadOnly() const override {
      return !isWriteable(); // as the accessor is always readable, isReadOnly() is equivalent to !isWriteable()
    }
    [[nodiscard]] bool isReadable() const override { return true; }
    [[nodiscard]] bool isWriteable() const override { return static_cast<bool>(_writeAccessor); }

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
    using typename NDRegisterAccessor<UserType>::Buffer;
    using NDRegisterAccessor<UserType>::buffer_2D;
    Buffer _receiveBuffer;

    // variables to simplify the bookkeeping and only send to the queue when it is allowed
    bool _isActive{false};
    // FIXME: This was taken from ExceptionDummyPushDecorator but I think it does not add any value unless there is
    // a scenario where it is allowed to send an exception before the accessor is activated
    // bool _hasException{false};

    cppext::future_queue<Buffer, cppext::SWAP_DATA> _dataTransportQueue{_queueSize};

    boost::shared_ptr<NDRegisterAccessor<UserType>> _writeAccessor;
  };

  /**********************************************************************************************************/
  /* Implementations are in the .cc file. We use the trick to declare for all known types here,             */
  /* and instantiate all of them in the .cc file. This is necessary because we cannot include               */
  /* AsyncAccessorManager.h to avoid a circular dependency, but need it for the implementation.             */
  /**********************************************************************************************************/
  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(AsyncNDRegisterAccessor);

} // namespace ChimeraTK
