// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AsyncNDRegisterAccessor.h"

#include "AsyncAccessorManager.h"

namespace ChimeraTK {

  template<typename UserType>
  AsyncNDRegisterAccessor<UserType>::~AsyncNDRegisterAccessor() {
    _accessorManager->unsubscribe(this->getId());
  }

  template<typename UserType>
  AsyncNDRegisterAccessor<UserType>::AsyncNDRegisterAccessor(boost::shared_ptr<DeviceBackend> backend,
      boost::shared_ptr<AsyncAccessorManager> manager, boost::shared_ptr<AsyncDomain> asyncDomain,
      std::string const& name, size_t nChannels, size_t nElements, AccessModeFlags accessModeFlags,
      std::string const& unit, std::string const& description)

  : NDRegisterAccessor<UserType>(name, accessModeFlags, unit, description), _backend(std::move(backend)),
    _accessorManager(std::move(manager)), _asyncDomain(std::move(asyncDomain)), _receiveBuffer(nChannels, nElements) {
    // Don't throw a ChimeraTK::logic_error here. They are for mistakes an application is doing when using DeviceAccess.
    // If an AsyncNDRegisterAccessor is created without wait_for_new_data it is a mistake in the backend, which is not
    // part of the application.
    assert(accessModeFlags.has(AccessMode::wait_for_new_data));
    buffer_2D.resize(nChannels);
    for(auto& chan : buffer_2D) chan.resize(nElements);

    // The sequence to initialise the queue:
    // * write once and then read,
    // * repeat n+1 times to make sure all buffers inside the queue have been replaced with
    //   a properly sized buffer, so it can be swapped out and used for data
    for(size_t i = 0; i < _queueSize + 1; ++i) {
      Buffer b1(nChannels, nElements);
      _dataTransportQueue.push(std::move(b1));
      Buffer b2(nChannels, nElements);
      _dataTransportQueue.pop(b2); // here b2 is swapped into the queue and transported "backwards"
    }

    this->_readQueue = _dataTransportQueue.template then<void>(
        [&](Buffer& buf) { std::swap(_receiveBuffer, buf); }, std::launch::deferred);
  }

  /**********************************************************************************************************/
  template<typename UserType>
  void AsyncNDRegisterAccessor<UserType>::doPostRead([[maybe_unused]] TransferType type, bool updateDataBuffer) {
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

  /**********************************************************************************************************/
  template<typename UserType>
  void AsyncNDRegisterAccessor<UserType>::sendDestructively(typename NDRegisterAccessor<UserType>::Buffer& data) {
    if(_asyncDomain->_isActive) {
      _dataTransportQueue.push_overwrite(std::move(data));
    }
  }

  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(AsyncNDRegisterAccessor);
} // namespace ChimeraTK
