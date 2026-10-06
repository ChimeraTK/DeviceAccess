// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DummyBackendBase.h"

#include <algorithm>
#include <regex>

using namespace std::string_literals;

namespace ChimeraTK {

  constexpr auto DUMMY_WRITEABLE_SUFFIX = "DUMMY_WRITEABLE";
  constexpr auto DUMMY_READABLE_SUFFIX = "DUMMY_READABLE";
  constexpr auto DUMMY_INTERRUPT_REGISTER_PREFIX = "/DUMMY_INTERRUPT_";

  /********************************************************************************************************************/

  DummyBackendBase::DummyBackendBase(std::string const& mapFileName, const std::string& dataConsistencyKeyDescriptor)
  : NumericAddressedBackend(
        mapFileName, std::make_unique<NumericAddressedRegisterCatalogue>(), dataConsistencyKeyDescriptor) {
    // Copy the old vtable
    OVERRIDE_VIRTUAL_FUNCTION_TEMPLATE(NumericAddressedBackend, getRegisterAccessor_impl);

    // Add dummy writeable for each read-only and dummy readable for each write-only register
    for(const auto& reg : _registerMap) {
      RegisterPath name = reg.pathName;
      if(reg.isReadable() && !reg.isWriteable()) {
        name /= DUMMY_WRITEABLE_SUFFIX;
      }
      else if(!reg.isReadable() && reg.isWriteable()) {
        name /= DUMMY_READABLE_SUFFIX;
      }
      else {
        // already readable + writeable -> do not add dummy register
        continue;
      }
      NumericAddressedRegisterInfo info = reg;
      info.pathName = name;
      info.registerAccess = NumericAddressedRegisterInfo::Access::READ_WRITE;
      info.hidden = true;
      _registerMap.addRegister(info);
    }

    // Add dummy interrupt registers for each primary interrupt
    for(const auto& interruptID : _registerMap.getListOfInterrupts()) {
      if(interruptID.size() == 1) {
        RegisterPath name = DUMMY_INTERRUPT_REGISTER_PREFIX + std::to_string(interruptID[0]);
        NumericAddressedRegisterInfo info(name, 0 /*nElements*/, 0 /*address*/, 0 /*nBytes*/, 0 /*bar*/, 0 /*width*/,
            0 /*facBits*/, false /*signed*/, NumericAddressedRegisterInfo::Access::WRITE_ONLY,
            NumericAddressedRegisterInfo::Type::VOID);
        info.hidden = true;
        _registerMap.addRegister(info);
      }
    }
  }

  /********************************************************************************************************************/

  size_t DummyBackendBase::minimumTransferAlignment([[maybe_unused]] uint64_t bar) const {
    return 4;
  }

  /********************************************************************************************************************/

  /// All bars are valid in dummies.
  bool DummyBackendBase::barIndexValid([[maybe_unused]] uint64_t bar) {
    return true;
  }

  /********************************************************************************************************************/

  void DummyBackendBase::transferChunked(uint64_t bar, uint64_t byteOffset, size_t sizeInBytes,
      const std::function<std::byte*(uint64_t, uint64_t)>& getChunk, const TransferCallback& transfer) {
    uint64_t remaining = sizeInBytes;
    uint64_t offset = byteOffset;
    uint64_t bytesDone = 0;
    while(remaining > 0) {
      const uint64_t chunkIndex = offset / CHUNK_SIZE;
      const uint64_t offsetInChunk = offset % CHUNK_SIZE;
      const size_t bytesInChunk = static_cast<size_t>(std::min<uint64_t>(remaining, CHUNK_SIZE - offsetInChunk));
      transfer(bytesInChunk, getChunk(bar, chunkIndex) + offsetInChunk, static_cast<size_t>(bytesDone));
      offset += bytesInChunk;
      bytesDone += bytesInChunk;
      remaining -= bytesInChunk;
    }
  }

  /********************************************************************************************************************/

  void DummyBackendBase::checkSizeIsMultipleOfWordSize(size_t sizeInBytes) {
    if(sizeInBytes % sizeof(int32_t)) {
      throw ChimeraTK::logic_error("Read/write size has to be a multiple of 4");
    }
  }

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> DummyBackendBase::getRegisterAccessor_impl(
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    // First check if the request is for one of the special DUMMY_INTERRUPT_X registers. if so, early return
    // this special accessor.
    if(registerPathName.startsWith("DUMMY_INTERRUPT_")) {
      bool interruptFound;
      uint32_t interrupt;

      std::tie(interruptFound, interrupt) = extractControllerInterrupt(registerPathName);
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

  /********************************************************************************************************************/

  std::pair<bool, int> DummyBackendBase::extractControllerInterrupt(const RegisterPath& registerPathName) const {
    static const std::string DUMMY_INTERRUPT_REGISTER_NAME{"^"s + DUMMY_INTERRUPT_REGISTER_PREFIX + "([0-9]+)$"};

    const std::string regPathNameStr{registerPathName};
    const static std::regex dummyInterruptRegex{DUMMY_INTERRUPT_REGISTER_NAME};
    std::smatch match;
    std::regex_search(regPathNameStr, match, dummyInterruptRegex);

    if(not match.empty()) {
      // FIXME: Ideally, this test and the need for passing in the lambda function should be done
      // in the constructor of the accessor. But passing down the base-class of the backend is very weird
      // due to the sort-of CRTP pattern used in this base class.
      auto primaryInterrupt = static_cast<unsigned int>(std::stoul(match[1].str()));
      for(const auto& interruptID : _registerMap.getListOfInterrupts()) {
        if(interruptID.front() == primaryInterrupt) {
          return {true, primaryInterrupt};
        }
      }
    }
    return {false, 0};
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
