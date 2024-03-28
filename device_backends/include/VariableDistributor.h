// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AsyncAccessorManager.h"
#include "InterruptControllerHandler.h"

#include <memory>

namespace ChimeraTK {

  template<typename SourceType>
  class VariableDistributor : public SourceTypedAsyncAccessorManager<SourceType> {
   public:
    VariableDistributor(boost::shared_ptr<DeviceBackend> backend,
        boost::shared_ptr<TriggerDistributor<SourceType>> parent, boost::shared_ptr<AsyncDomain> asyncDomain);

    template<typename UserType>
    std::unique_ptr<AsyncVariable> createAsyncVariable(AccessorInstanceDescriptor const& descriptor);

    boost::shared_ptr<TriggerDistributor<SourceType>> _parent;
  };

  /********************************************************************************************************************/

  template<typename SourceType, typename UserType>
  class GenericAsyncVariable : public AsyncVariableImpl<UserType> {
   public:
    GenericAsyncVariable(SourceType& dataBuffer, VersionNumber& v, unsigned int nChannels, unsigned int nElements)
    : AsyncVariableImpl<UserType>(nChannels, nElements), _dataBuffer(dataBuffer), _version(v) {}

    // implement fillSendBuffer() in a derrived class for template specialisation

    /** Make template specialisations on the SourceType in case the source data contains a unit.
     */
    const std::string& getUnit() override { return _emptyString; }

    /** Make template specialisations on the SourceType in case the source data contains a description.
     */
    const std::string& getDescription() override { return _emptyString; }

   protected:
    std::string _emptyString{};
    SourceType& _dataBuffer;
    VersionNumber& _version;
  };

  /********************************************************************************************************************/

  // partial template specialisation by inheritance for void
  template<typename UserType>
  class VoidAsyncVariable : public GenericAsyncVariable<std::nullptr_t, UserType> {
    using GenericAsyncVariable<std::nullptr_t, UserType>::GenericAsyncVariable;
    void fillSendBuffer() final;
  };

  /********************************************************************************************************************/
  // Implementations
  /********************************************************************************************************************/
  template<typename SourceType>
  VariableDistributor<SourceType>::VariableDistributor(boost::shared_ptr<DeviceBackend> backend,
      boost::shared_ptr<TriggerDistributor<SourceType>> parent, boost::shared_ptr<AsyncDomain> asyncDomain)
  : SourceTypedAsyncAccessorManager<SourceType>(backend, asyncDomain), _parent(std::move(parent)) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(createAsyncVariable);
  }

  /********************************************************************************************************************/

  // currently only for void
  template<>
  template<typename UserType>
  std::unique_ptr<AsyncVariable> VariableDistributor<std::nullptr_t>::createAsyncVariable(
      AccessorInstanceDescriptor const&) {
    // for the full implementation
    // - extract size from catalogue and instance descriptor
    return std::make_unique<VoidAsyncVariable<UserType>>(_sourceBuffer, _version, 1, 1);
  }

  /********************************************************************************************************************/
  template<typename UserType>
  void VoidAsyncVariable<UserType>::fillSendBuffer() {
    // We know that the SourceBuffer contains nullptr. We don't have a conversion formula for that to user type
    // (especially for string). But we know how to convert ChimeraTK::Void, so we do this instead.
    this->_sendBuffer.value[0][0] = userTypeToUserType<UserType, ChimeraTK::Void>({});
    this->_sendBuffer.versionNumber = this->_version;
  }

} // namespace ChimeraTK
