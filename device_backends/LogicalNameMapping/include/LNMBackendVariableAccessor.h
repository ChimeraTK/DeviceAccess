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
#include "NDRegisterAccessor.h"

namespace ChimeraTK {

  /*********************************************************************************************************************/

  template<typename UserType>
  class LNMBackendVariableAccessor : public NDRegisterAccessor<UserType> {
   public:
    LNMBackendVariableAccessor(boost::shared_ptr<DeviceBackend> dev, const RegisterPath& registerPathName,
        size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags)
    : NDRegisterAccessor<UserType>(registerPathName, flags), _registerPathName(registerPathName),
      _wordOffsetInRegister(wordOffsetInRegister), _fixedPointConverter(registerPathName, 32, 0, 1) {
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
        throw ChimeraTK::logic_error("AccessMode::raw is not supported on a "
                                     "variable/constant-type register in a logical "
                                     "name mapping device.");
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
      fillUserBuffer();
    }

    void doReadTransferSynchronously() override {
      if(_dev->_hasException) {
        throw ChimeraTK::runtime_error("previous, unrecovered fault");
      }
    }

    void doPreWrite(TransferType type, VersionNumber) override {
      std::ignore = type;
      if(_info->targetType ==
          LNMBackendRegisterInfo::TargetType::
              CONSTANT) { // repeat the condition in isReadOnly() here to avoid virtual function call.
        throw ChimeraTK::logic_error("Writing to constant-type registers of logical name mapping devices "
                                     "is not possible.");
      }
      if(!_dev->_opened) { // directly use member variables as friend to avoid virtual function calls
        throw ChimeraTK::logic_error("Cannot write to a closed device.");
      }

      _valueTableDataValidity = this->_dataValidity;
    }

    bool doWriteTransfer(ChimeraTK::VersionNumber) override {
      if(_dev->_hasException) {
        throw ChimeraTK::runtime_error("previous, unrecovered fault");
      }
      std::lock_guard<std::mutex> lock(_info->valueTable_mutex);
      for(size_t i = 0; i < NDRegisterAccessor<UserType>::buffer_2D[0].size(); ++i) {
        callForType(_info->valueType, [&, this](auto arg) {
          boost::fusion::at_key<decltype(arg)>(_info->valueTable.table)[i + _wordOffsetInRegister] =
              userTypeToUserType<decltype(arg)>(this->buffer_2D[0][i]);
        });
      }
      return false;
    }

    bool mayReplaceOther(const boost::shared_ptr<TransferElement const>&) const override {
      return false; // never replace, since it does not optimise anything
    }

    bool isReadOnly() const override { return _info->targetType == LNMBackendRegisterInfo::TargetType::CONSTANT; }

    bool isReadable() const override { return true; }

    bool isWriteable() const override { return _info->targetType != LNMBackendRegisterInfo::TargetType::CONSTANT; }

    void fillUserBuffer() {
      std::lock_guard<std::mutex> lock(_info->valueTable_mutex);
      for(size_t i = 0; i < NDRegisterAccessor<UserType>::buffer_2D[0].size(); ++i) {
        callForType(_info->valueType, [&, this](auto arg) {
          this->buffer_2D[0][i] = userTypeToUserType<UserType>(
              boost::fusion::at_key<decltype(arg)>(_info->valueTable.table)[i + _wordOffsetInRegister]);
        });
      }
    }

    void doPreRead(TransferType) override {
      if(!_dev->_opened) {
        throw ChimeraTK::logic_error("Cannot read from a closed device.");
      }
    }

    void doPostRead(TransferType, bool hasNewData) override {
      if(!hasNewData) return;
      fillUserBuffer();
      this->_versionNumber = {};
      this->_dataValidity = _valueTableDataValidity;
    }

   protected:
    /// register and module name
    RegisterPath _registerPathName;

    /// backend device
    boost::shared_ptr<LogicalNameMappingBackend> _dev;

    /// register information. We have a shared pointer to the original
    /// RegisterInfo inside the map, since we need to modify the value in it (in
    /// case of a writeable variable register)
    boost::shared_ptr<LNMBackendRegisterInfo> _info;

    /// The validity of the data in the value table of _info.
    DataValidity _valueTableDataValidity{DataValidity::ok};

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
    VersionNumber currentVersion{nullptr};
  };

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(LNMBackendVariableAccessor);

} // namespace ChimeraTK

#endif /* CHIMERA_TK_LNM_BACKEND_BUFFERING_VARIABLE_ACCESSOR_H */
