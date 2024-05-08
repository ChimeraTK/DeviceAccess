// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <DummyBackendBase.h>

namespace ChimeraTK {

  DummyBackendBase::DummyBackendBase(std::string const& mapFileName)
  : NumericAddressedBackend(mapFileName, std::make_unique<DummyBackendRegisterCatalogue>()) {
    // Copy the old vtable
    OVERRIDE_VIRTUAL_FUNCTION_TEMPLATE(NumericAddressedBackend, getRegisterAccessor_impl);
  }

  size_t DummyBackendBase::minimumTransferAlignment([[maybe_unused]] uint64_t bar) const {
    return 4;
  }

  /// All bars are valid in dummies.
  bool DummyBackendBase::barIndexValid([[maybe_unused]] uint64_t bar) {
    return true;
  }

  std::map<uint64_t, size_t> DummyBackendBase::getBarSizesInBytesFromRegisterMapping() const {
    std::map<uint64_t, size_t> barSizesInBytes;
    for(auto const& info : _registerMap) {
      if(info.elementPitchBits % 8 != 0) {
        throw ChimeraTK::logic_error("DummyBackendBase: Elements have to be byte aligned.");
      }
      barSizesInBytes[info.bar] = std::max(
          barSizesInBytes[info.bar], static_cast<size_t>(info.address + info.nElements * info.elementPitchBits / 8));
    }
    return barSizesInBytes;
  }

  void DummyBackendBase::checkSizeIsMultipleOfWordSize(size_t sizeInBytes) {
    if(sizeInBytes % sizeof(int32_t)) {
      throw ChimeraTK::logic_error("Read/write size has to be a multiple of 4");
    }
  }

  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> DummyBackendBase::getRegisterAccessor_impl(
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    // First check if the request is for one of the special DUMMY_INTEERRUPT_X registers. if so, early return
    // this special accessor.
    if(registerPathName.startsWith("DUMMY_INTERRUPT_")) {
      bool interruptFound;
      uint32_t interrupt;

      auto* dummyCatalogue = dynamic_cast<DummyBackendRegisterCatalogue*>(_registerMapPointer.get());
      assert(dummyCatalogue);
      std::tie(interruptFound, interrupt) = dummyCatalogue->extractControllerInterrupt(registerPathName);
      if(!interruptFound) {
        throw ChimeraTK::logic_error("Unknown dummy interrupt " + registerPathName);
      }

      // Delegate the other parameters down to the accessor which will throw accordingly, to satisfy the specification
      // Since the accessor will keep a shared pointer to the backend, we can safely capture "this"
      auto d = new DummyInterruptTriggerAccessor<UserType>(
          shared_from_this(), [this, interrupt]() { return triggerInterrupt(interrupt); }, registerPathName,
          numberOfWords, wordOffsetInRegister, flags);

      return boost::shared_ptr<NDRegisterAccessor<UserType>>(d);
    }

    // Chain up to to the base class using the previously stored function
    return CALL_BASE_FUNCTION_TEMPLATE(NumericAddressedBackend, getRegisterAccessor_impl, UserType, registerPathName,
        numberOfWords, wordOffsetInRegister, flags);
  }

} // namespace ChimeraTK
