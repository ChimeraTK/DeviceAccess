// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <DummyBackendBase.h>

namespace ChimeraTK {

  DummyBackendBase::DummyBackendBase(std::string const& mapFileName)
  : NumericAddressedBackend(mapFileName, std::make_unique<DummyBackendRegisterCatalogue>()) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl);
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

} // namespace ChimeraTK
