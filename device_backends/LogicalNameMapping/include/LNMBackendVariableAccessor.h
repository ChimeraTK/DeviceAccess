/*
 * LNMBackendBufferingVariableAccessor.h - Access a variable or constant in a
 * logical name mapping file with a buffering-type accessor.
 *
 *  Created on: Feb 16, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_LNM_BACKEND_BUFFERING_VARIABLE_ACCESSOR_H
#define CHIMERA_TK_LNM_BACKEND_BUFFERING_VARIABLE_ACCESSOR_H

#include <algorithm>

#include "BufferingRegisterAccessor.h"
#include "Device.h"
#include "FixedPointConverter.h"
#include "LogicalNameMappingBackend.h"
#include "SyncNDRegisterAccessor.h"

namespace ChimeraTK {

  /*********************************************************************************************************************/

  template<typename UserType>
  class LNMBackendVariableAccessor : public SyncNDRegisterAccessor<UserType> {
   public:
    LNMBackendVariableAccessor(boost::shared_ptr<DeviceBackend> dev, const RegisterPath& registerPathName,
        size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags)
    : SyncNDRegisterAccessor<UserType>(registerPathName), _registerPathName(registerPathName),
      _wordOffsetInRegister(wordOffsetInRegister), _fixedPointConverter(registerPathName, 32, 0, 1) {
      try {
        // cast device
        _dev = boost::dynamic_pointer_cast<LogicalNameMappingBackend>(dev);
        // check for unknown flags
        flags.checkForUnknownFlags({AccessMode::raw});
        // obtain the register info
        _info = boost::static_pointer_cast<LNMBackendRegisterInfo>(
            _dev->getRegisterCatalogue().getRegister(_registerPathName));
        if(numberOfWords == 0) numberOfWords = _info->length;
        // check for illegal parameter combinations
        if(wordOffsetInRegister + numberOfWords > _info->length) {
          throw ChimeraTK::logic_error("Requested number of words and/or offset "
                                       "exceeds length of register '" +
              registerPathName + "'.");
        }
        if(flags.has(AccessMode::raw)) {
          if(typeid(UserType) != typeid(int32_t)) {
            throw ChimeraTK::logic_error("Given UserType when obtaining the "
                                         "LNMBackendBufferingVariableAccessor in"
                                         " raw mode does not match the expected "
                                         "type. Use an int32_t instead!");
          }
          std::cout << "WARNING: You are using AccessMode::raw on a "
                       "variable/constant-type register in a logical "
                       "name mapping device. This is DEPRECATED and support for "
                       "it will be removed soon!"
                    << std::endl;
        }
        // check for incorrect usage of this accessor
        if(_info->targetType != LNMBackendRegisterInfo::TargetType::CONSTANT &&
            _info->targetType != LNMBackendRegisterInfo::TargetType::VARIABLE) {
          throw ChimeraTK::logic_error("LNMBackendVariableAccessor used for "
                                       "wrong register type."); // LCOV_EXCL_LINE
                                                                // (impossible to
                                                                // test...)
        }
        NDRegisterAccessor<UserType>::buffer_2D.resize(1);
        NDRegisterAccessor<UserType>::buffer_2D[0].resize(numberOfWords);
        doPostRead();
      }
      catch(...) {
        this->shutdown();
        throw;
      }
    }

    virtual ~LNMBackendVariableAccessor() override { this->shutdown(); }

    void doReadTransfer() override {}

    bool doReadTransferNonBlocking() override { return true; }

    bool doReadTransferLatest() override { return true; }

    bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber = {}) override {
      if(isReadOnly()) {
        throw ChimeraTK::logic_error("Writing to constant-type registers of logical name mapping devices "
                                     "is not possible.");
      }
      std::lock_guard<std::mutex> lock(_info->valueTable_mutex);
      for(size_t i = 0; i < NDRegisterAccessor<UserType>::buffer_2D[0].size(); ++i) {
        callForType(_info->valueType, [&, this](auto arg) {
          boost::fusion::at_key<decltype(arg)>(_info->valueTable.table)[i + _wordOffsetInRegister] =
              userTypeToUserType<decltype(arg)>(this->buffer_2D[0][i]);
        });
      }
      currentVersion = versionNumber;
      return false;
    }

    bool mayReplaceOther(const boost::shared_ptr<TransferElement const>&) const override {
      return false; // never replace, since it does not optimise anything
    }

    bool isReadOnly() const override { return _info->targetType == LNMBackendRegisterInfo::TargetType::CONSTANT; }

    bool isReadable() const override { return true; }

    bool isWriteable() const override { return _info->targetType != LNMBackendRegisterInfo::TargetType::CONSTANT; }

    void doPostRead() override {
      std::lock_guard<std::mutex> lock(_info->valueTable_mutex);
      for(size_t i = 0; i < NDRegisterAccessor<UserType>::buffer_2D[0].size(); ++i) {
        callForType(_info->valueType, [&, this](auto arg) {
          this->buffer_2D[0][i] = userTypeToUserType<UserType>(
              boost::fusion::at_key<decltype(arg)>(_info->valueTable.table)[i + _wordOffsetInRegister]);
        });
      }
      currentVersion = {};
    }

    AccessModeFlags getAccessModeFlags() const override { return {}; }

    VersionNumber getVersionNumber() const override { return currentVersion; }

   protected:
    /// register and module name
    RegisterPath _registerPathName;

    /// backend device
    boost::shared_ptr<LogicalNameMappingBackend> _dev;

    /// register information. We have a shared pointer to the original
    /// RegisterInfo inside the map, since we need to modify the value in it (in
    /// case of a writeable variable register)
    boost::shared_ptr<LNMBackendRegisterInfo> _info;

    /// Word offset when reading
    size_t _wordOffsetInRegister;

    /// fixed point converter to handle type conversions from our "raw" type int
    /// to the requested user type. Note: no actual fixed point conversion is
    /// done, it is just used for the type conversion!
    FixedPointConverter _fixedPointConverter;

    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override {
      return {boost::enable_shared_from_this<TransferElement>::shared_from_this()};
    }

    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override { return {}; }

    void replaceTransferElement(boost::shared_ptr<TransferElement> /*newElement*/) override {} // LCOV_EXCL_LINE

    // Version number of the last transfer
    VersionNumber currentVersion;
  };

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(LNMBackendVariableAccessor);

} // namespace ChimeraTK

#endif /* CHIMERA_TK_LNM_BACKEND_BUFFERING_VARIABLE_ACCESSOR_H */
