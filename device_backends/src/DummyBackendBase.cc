#include <DummyBackendBase.h>

namespace ChimeraTK {

  DummyBackendBase::DummyBackendBase(std::string const& mapFileName)
  : NumericAddressedBackend(mapFileName), _registerMapping{_registerMap} {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl);
  }

  DummyBackendBase::~DummyBackendBase() {}

  size_t DummyBackendBase::minimumTransferAlignment([[maybe_unused]] uint64_t bar) const { return 4; }

  /// All bars are valid in dummies.
  bool DummyBackendBase::barIndexValid([[maybe_unused]] uint64_t bar) { return true; }

  std::map<uint64_t, size_t> DummyBackendBase::getBarSizesInBytesFromRegisterMapping() const {
    std::map<uint64_t, size_t> barSizesInBytes;
    for(RegisterInfoMap::const_iterator mappingElementIter = _registerMapping->begin();
        mappingElementIter != _registerMapping->end(); ++mappingElementIter) {
      barSizesInBytes[mappingElementIter->bar] = std::max(barSizesInBytes[mappingElementIter->bar],
          static_cast<size_t>(mappingElementIter->address + mappingElementIter->nBytes));
    }
    return barSizesInBytes;
  }

  void DummyBackendBase::checkSizeIsMultipleOfWordSize(size_t sizeInBytes) {
    if(sizeInBytes % sizeof(int32_t)) {
      throw ChimeraTK::logic_error("Read/write size has to be a multiple of 4");
    }
  }

} //namespace ChimeraTK
