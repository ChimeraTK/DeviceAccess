#pragma once

#include "NDRegisterAccessor.h"
#include "DeviceBackend.h"
#include <ChimeraTK/cppext/future_queue.hpp>

namespace ChimeraTK {

  template<typename UserType, typename AsyncAccessorManager>
  class AsyncNDRegisterAccessor
  : public NDRegisterAccessor<UserType>,
    public boost::enable_shared_from_this<AsyncNDRegisterAccessor<UserType, AsyncAccessorManager>> {
   public:
    AsyncNDRegisterAccessor(const boost::shared_ptr<DeviceBackend>& backend,
        const boost::shared_ptr<AsyncAccessorManager>& manager, std::string const& name, size_t nChannels,
        size_t nElements, AccessModeFlags accessModeFlags,
        std::string const& unit = std::string(TransferElement::unitNotSet),
        std::string const& description = std::string())

    : NDRegisterAccessor<UserType>(name, accessModeFlags, unit, description), _backend(backend),
      _accessorManager(manager), _receiveBuffer(nChannels, nElements) {
      if(!accessModeFlags.has(AccessMode::wait_for_new_data)) {
        throw ChimeraTK::logic_error("AsyncNDRegisterAccessor requested without AccessMode::wait_for_new_data");
      }
      buffer_2D.resize(nChannels);
      for(auto& chan : buffer_2D) chan.resize(nElements);

      this->_readQueue = _dataTransportQueue.template then<void>(
          [&](Buffer buf) { std::swap(_receiveBuffer, buf); }, std::launch::deferred);
      _accessorManager->subscribe(this->shared_from_this());
    }

    ~AsyncNDRegisterAccessor() override { _accessorManager->unsubscribe(this->shared_from_this()); }

    void doReadTransferSynchronously() override {
      throw ChimeraTK::logic_error("AsyncNDRegisterAccessor does not support synchronous reads.");
    }

    void doPreWrite([[maybe_unused]] TransferType type, [[maybe_unused]] VersionNumber versionNumber) override {
      throw ChimeraTK::logic_error("AsyncNDRegisterAccessor does not support writing.");
    }

    void doPreRead([[maybe_unused]] TransferType type) override {
      if(!_backend->isOpen()) throw ChimeraTK::logic_error("Device not opened.");
      // Pre-read conceptually does nothing in synchronous reads
    }

    void doPostWrite([[maybe_unused]] TransferType type, [[maybe_unused]] VersionNumber versionNumber) override {
      throw ChimeraTK::logic_error("AsyncNDRegisterAccessor does not support writing.");
    }

    void doPostRead(TransferType, bool updateDataBuffer) override;

    bool isReadOnly() const override { return true; }
    bool isReadable() const override { return true; }
    bool isWriteable() const override { return false; }

    void setExceptionBackend(boost::shared_ptr<DeviceBackend> exceptionBackend) override {
      this->_exceptionBackend = exceptionBackend;
    }

    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() {
      return {boost::enable_shared_from_this<TransferElement>::shared_from_this()};
    }
    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override { return {}; }

    void replaceTransferElement(boost::shared_ptr<TransferElement> /*newElement*/) override {} // LCOV_EXCL_LINE

    void interrupt() override { this->interrupt_impl(this->_dataQueue); }

    void sendException(std::exception_ptr& e) {
      if(_isActive) { //FIXME: is there a scenario where we have to/are allowed to send an exception when the accessor is inactive?
        _dataTransportQueue.push_overwrite_exception(e);
      }
      _isActive = false;
      //_hasException = true;
    }

    /** You can only send descructively. If you want to keep a copy you have to make once yourself.
     *  This is more efficient that having one extra buffer within each AsyncNDRegisterAccessor.
     */
    void sendDestrictively(typename NDRegisterAccessor<UserType>::Buffer& data) {
      if(!_isActive) { // || _hasException) {
        return;
      }

      _dataTransportQueue.push_overwrite(data);
    }

    /** Activate the accessor and send the initial value.
    */
    void activate(typename NDRegisterAccessor<UserType>::Buffer& initialValue) {
      //_hasException = false;
      _isActive = true;
      send(initialValue);
    }

    // Needs to be called in case a device is closed.
    void deactivate() { _isActive = false; }

   protected:
    boost::shared_ptr<DeviceBackend> _backend;
    boost::shared_ptr<AsyncAccessorManager> _accessorManager;
    using typename NDRegisterAccessor<UserType>::Buffer;
    using NDRegisterAccessor<UserType>::buffer_2D;
    Buffer& _receiveBuffer;

    // variables to simplify the bookkeeping and only send to the queue when it is allowed
    bool _isActive{false};
    // FIXME: This was taken from ExceptionDummyPushDecorator but I think it does not add any value unless there is
    // a scenario where it is allowed to send an exception before the accessor is activated
    // bool _hasException{false};

    cppext::future_queue<Buffer, cppext::SWAP_DATA> _dataTransportQueue{3};
  }; // namespace ChimeraTK

  template<typename UserType, typename AsyncAccessorManager>
  void AsyncNDRegisterAccessor<UserType, AsyncAccessorManager>::doPostRead(
      [[maybe_unused]] TransferType type, bool updateDataBuffer) {
    if(updateDataBuffer) {
      // do not update meta data if updateDataBuffer == false, since this is the equivalent to a backend
      // implementation, not a decorator
      this->_versionNumber = _receiveBuffer.version;
      this->_dataValidity = _receiveBuffer.validity;
      // Do not overwrite the vectors in the first layer of the 2D array. Accessing code might have stored them.
      // Instead, swap the received data into the channel vectors.
      auto source = _receiveBuffer.data.begin(); // the received data is the source as it is moved into the user buffer
      auto destination = this->buffer_2D.begin();
      for(; source != _receiveBuffer.data.end(); ++source, ++destination) {
        destination.swap(source);
      }
    }
  }
} // namespace ChimeraTK
