#pragma once

#include "NDRegisterAccessor.h"
#include "DeviceBackend.h"
#include <ChimeraTK/cppext/future_queue.hpp>

namespace ChimeraTK {

  /** The AsyncNDRegisterAccessor implements a data transport queue with typed data
   *  as continuation of the void queue in TransferElement. This allows to
   *  receive the content of the buffer_2D, the version number and the data validity flag.
   *  The implementation is complete. The interrupt handling thread in the backend implementation
   *  can write to the queues through the memeber functions, and activate and deactivate the accessor.
   *
   *  To allow clean subscription of asynchronous accessors in the backend, the
   *  AsyncNDRegisterAccessor is templated to an AsyncAccessorManager class and an AsyncAccessorID.
   *  The accessor should be registed to the manager before given out when it is created. The
   *  destructor of the AsyncNDRegisterAccessor will call a function
   *  <pre>
void AsyncAccessorManager::unsubscribe(AsyncAccessorID)
</pre>
   *  in its destructor to unregister itself. As the way the asynchronous accessors are store
   *  highly depends on the interrupt and data structure of backend, the implementation of the
   *  manager is up to the backend implementation.
   */
  template<typename UserType, typename AsyncAccessorManager, typename AsyncAccessorID>
  class AsyncNDRegisterAccessor : public NDRegisterAccessor<UserType> {
   public:
    /** In addition to the arguments of the NDRegisterAccessor constructor, you need
       *  an AccessorManager where you can unsubscribe, and an ID so the AccessorManager
       *  can identify the accessor when doing so.
       */
    AsyncNDRegisterAccessor(const boost::shared_ptr<DeviceBackend>& backend,
        const boost::shared_ptr<AsyncAccessorManager>& manager, std::string const& name, size_t nChannels,
        size_t nElements, AccessModeFlags accessModeFlags, AsyncAccessorID id,
        std::string const& unit = std::string(TransferElement::unitNotSet),
        std::string const& description = std::string());

    ~AsyncNDRegisterAccessor() override { _accessorManager->unsubscribe(_id); }

    /** Pushes the exception to the queue and marks the accessor as inactive, so nothing
     *  is pushed until re-activation.
     */
    void sendException(std::exception_ptr& e) {
      if(_isActive) {
        _dataTransportQueue.push_overwrite_exception(e);
      }
      _isActive = false;
    }

    /** You can only send descructively. If you want to keep a copy you have to make once yourself.
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

    ////////////////////////////////////////////////////
    // implementation of inherited, virtual functions //
    ////////////////////////////////////////////////////

    void doReadTransferSynchronously() override {
      throw ChimeraTK::logic_error("AsyncNDRegisterAccessor does not support synchronous reads.");
    }

    bool doWriteTransfer(ChimeraTK::VersionNumber) override {
      throw ChimeraTK::logic_error("AsyncNDRegisterAccessor does not support writing.");
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

    void interrupt() override { this->interrupt_impl(this->_dataTransportQueue); }

   protected:
    boost::shared_ptr<DeviceBackend> _backend;
    boost::shared_ptr<AsyncAccessorManager> _accessorManager;
    using typename NDRegisterAccessor<UserType>::Buffer;
    using NDRegisterAccessor<UserType>::buffer_2D;
    Buffer _receiveBuffer;
    AsyncAccessorID _id;

    // variables to simplify the bookkeeping and only send to the queue when it is allowed
    bool _isActive{false};
    // FIXME: This was taken from ExceptionDummyPushDecorator but I think it does not add any value unless there is
    // a scenario where it is allowed to send an exception before the accessor is activated
    // bool _hasException{false};

    cppext::future_queue<Buffer, cppext::SWAP_DATA> _dataTransportQueue{3};
  };

  /**********************************************************************************************************/
  /* Implementations                                                                                        */
  /**********************************************************************************************************/

  template<typename UserType, typename AsyncAccessorManager, typename AsyncAccessorID>
  AsyncNDRegisterAccessor<UserType, AsyncAccessorManager, AsyncAccessorID>::AsyncNDRegisterAccessor(
      const boost::shared_ptr<DeviceBackend>& backend, const boost::shared_ptr<AsyncAccessorManager>& manager,
      std::string const& name, size_t nChannels, size_t nElements, AccessModeFlags accessModeFlags, AsyncAccessorID id,
      std::string const& unit, std::string const& description)

  : NDRegisterAccessor<UserType>(name, accessModeFlags, unit, description), _backend(backend),
    _accessorManager(manager), _receiveBuffer(nChannels, nElements), _id(id) {
    if(!accessModeFlags.has(AccessMode::wait_for_new_data)) {
      throw ChimeraTK::logic_error("AsyncNDRegisterAccessor requested without AccessMode::wait_for_new_data");
    }
    buffer_2D.resize(nChannels);
    for(auto& chan : buffer_2D) chan.resize(nElements);

    this->_readQueue = _dataTransportQueue.template then<void>(
        [&](Buffer& buf) { std::swap(_receiveBuffer, buf); }, std::launch::deferred);
  }

  /**********************************************************************************************************/
  template<typename UserType, typename AsyncAccessorManager, typename AsyncAccessorID>
  void AsyncNDRegisterAccessor<UserType, AsyncAccessorManager, AsyncAccessorID>::doPostRead(
      [[maybe_unused]] TransferType type, bool updateDataBuffer) {
    if(updateDataBuffer) {
      // do not update meta data if updateDataBuffer == false, since this is the equivalent to a backend
      // implementation, not a decorator
      this->_versionNumber = _receiveBuffer.versionNumber;
      this->_dataValidity = _receiveBuffer.dataValidity;
      // Do not overwrite the vectors in the first layer of the 2D array. Accessing code might have stored them.
      // Instead, swap the received data into the channel vectors.
      auto source = _receiveBuffer.value.begin(); // the received data is the source as it is moved into the user buffer
      auto destination = this->buffer_2D.begin();
      for(; source != _receiveBuffer.value.end(); ++source, ++destination) {
        destination->swap(*source);
      }
    }
  }

} // namespace ChimeraTK
