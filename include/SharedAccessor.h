// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "CountedRecursiveMutex.h"
#include "DeviceBackend.h"
#include "NDRegisterAccessor.h"
#include "TransferElementID.h"
#include "VariantUserTypes.h"

#include <mutex>

namespace ChimeraTK::detail {

  using SharedAccessorKey = std::pair<DeviceBackend*, RegisterPath>;

  /** Map of target accessors which are potentially shared across accessors. An example is the target accessors of
   *  LNMBackendBitAccessor. Multiple instances of LNMBackendBitAccessor referring to different bits of the same
   *  register share their target accessor. This sharing is governed by this map. */
  class SharedAccessors {
   public:
    static SharedAccessors& getInstance();

    /** SharedState of a (complete) target register.
     *  It contains the complete data buffer and a mutex to protect it.
     * The mutex must also be held while performing any operation on an accessor registered in the
     * _transferSharedStates map.
     */
    struct TargetSharedState {
      // Helper name for the variant because the template argument is on NDRegisterAccessor, not the buffer class
      // itself.
      template<typename T>
      using UserBuffer = NDRegisterAccessor<T>::Buffer;

      detail::CountedRecursiveMutex mutex;
      UserTypeTemplateVariant<UserBuffer> dataBuffer;
    };

    /** SharedState for all accessors sharing the same TransferElement.
At the mmomen*/
    struct TransferSharedState {
      size_t instanceCount;
    };

    /**
     * Shared pointer to the TargetSharedState for the corresponding key.
     * The UserType is required for the initialisation of the data buffer variant. If the data buffer is already
     * initialised, the UserType is checked for consistency. A ChimeraTK::logic_error it thrown in case the requested
     * type does not match with the existing buffer.
     *
     */
    template<typename UserType>
    std::shared_ptr<TargetSharedState> getTargetSharedState(SharedAccessorKey const& key);

    void combineTransferSharedStates(TransferElementID oldId, TransferElementID newId);
    void addTransferElement(TransferElementID id);
    void removeTransferElement(TransferElementID id);
    size_t instanceCount(TransferElementID id);

   protected:
    std::mutex _mapMutex;
    // The key to the TargetSharedState is a shared_ptr because we give it out to be stored. Direct pointers/references
    // to the value objects of the map are not safe against insertions into the map.
    std::map<SharedAccessorKey, std::shared_ptr<TargetSharedState>> _targetSharedStates;
    std::map<TransferElementID, TransferSharedState> _transferSharedStates;

   private:
    SharedAccessors() = default;
  };

  /********************************************************************************************************************/

  template<typename UserType>
  std::shared_ptr<SharedAccessors::TargetSharedState> SharedAccessors::getTargetSharedState(
      SharedAccessorKey const& key) {
    std::lock_guard<std::mutex> l(_mapMutex); // protect against concurrent map insertion
    auto& tss = _targetSharedStates[key];
    if(tss == nullptr) {
      tss = std::make_shared<TargetSharedState>();

      auto registerInfo = key.first->getRegisterCatalogue().getRegister(key.second);
      tss->dataBuffer = typename NDRegisterAccessor<UserType>::Buffer(
          registerInfo.getNumberOfChannels(), registerInfo.getNumberOfElements());
    }
    else {
      // check that requested and existing user type are matching
      auto _sharedBuffer = std::get_if<typename NDRegisterAccessor<UserType>::Buffer>(&(tss->dataBuffer));
      if(_sharedBuffer == nullptr) {
        auto print_variant_type = [](auto&& value) {
          using T = std::decay_t<decltype(value)>;
          return boost::core::demangle(typeid(T).name());
        };

        throw ChimeraTK::logic_error("SubArrayAccessorDecorator for " + key.second + ": Requested TargetUserType '" +
            boost::core::demangle(typeid(UserType).name()) +
            "' does not match already existing type. Variant type is '" +
            std::visit(print_variant_type, tss->dataBuffer) + "'");
      }
    }

    return tss;
  }

} // namespace ChimeraTK::detail
