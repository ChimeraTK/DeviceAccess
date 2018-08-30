/*
 * LNMBackendBufferingVariableAccessor.h - Access a variable or constant in a logical name mapping file with a
 *                                         buffering-type accessor.
 *
 *  Created on: Feb 16, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_LNM_BACKEND_BUFFERING_VARIABLE_ACCESSOR_H
#define CHIMERA_TK_LNM_BACKEND_BUFFERING_VARIABLE_ACCESSOR_H

#include <algorithm>

#include "LogicalNameMappingBackend.h"
#include "BufferingRegisterAccessor.h"
#include "FixedPointConverter.h"
#include "Device.h"
#include "SyncNDRegisterAccessor.h"

namespace ChimeraTK {

  /*********************************************************************************************************************/

  template<typename UserType>
  class LNMBackendVariableAccessor : public SyncNDRegisterAccessor<UserType> {
    public:

      LNMBackendVariableAccessor(boost::shared_ptr<DeviceBackend> dev, const RegisterPath &registerPathName,
          size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags)
      : SyncNDRegisterAccessor<UserType>(registerPathName),
        _registerPathName(registerPathName),
        _fixedPointConverter(registerPathName, 32, 0, 1)
      {
        try {
          // check for unknown flags
          flags.checkForUnknownFlags({AccessMode::raw});
          // check for illegal parameter combinations
          if(wordOffsetInRegister != 0 || numberOfWords > 1) {
            throw DeviceException("LNMBackendBufferingVariableAccessor: offset and number of words not "
                "supported!", DeviceException::NOT_IMPLEMENTED); // LCOV_EXCL_LINE (impossible to test...)
          }
          if(flags.has(AccessMode::raw)) {
            if(typeid(UserType) != typeid(int32_t)) {
              throw DeviceException("Given UserType when obtaining the LNMBackendBufferingVariableAccessor in raw mode"
                  " does not match the expected type. Use an int32_t instead!", DeviceException::WRONG_PARAMETER);
            }
          }
          _dev = boost::dynamic_pointer_cast<LogicalNameMappingBackend>(dev);
          // obtain the register info
          _info = boost::static_pointer_cast<LNMBackendRegisterInfo>(
              _dev->getRegisterCatalogue().getRegister(_registerPathName));
          // check for incorrect usage of this accessor
          if( _info->targetType != LNMBackendRegisterInfo::TargetType::INT_CONSTANT &&
              _info->targetType != LNMBackendRegisterInfo::TargetType::INT_VARIABLE    ) {
            throw DeviceException("LNMBackendBufferingVariableAccessor used for wrong register type.",
                DeviceException::WRONG_PARAMETER); // LCOV_EXCL_LINE (impossible to test...)
          }
          NDRegisterAccessor<UserType>::buffer_2D.resize(1);
          NDRegisterAccessor<UserType>::buffer_2D[0].resize(_info->length);
          size_t m = _info->length;
          if(_info->value_int.size() < m) _info->value_int.size();
          for(size_t i=0; i < m; ++i) {
            NDRegisterAccessor<UserType>::buffer_2D[0][i] = _fixedPointConverter.toCooked<UserType>(_info->value_int[i]);
          }
        }
        catch(...) {
          this->shutdown();
          throw;
        }
      }

      virtual ~LNMBackendVariableAccessor() override { this->shutdown(); }

      void doReadTransfer() override {}

      bool doReadTransferNonBlocking() override {
        return true;
      }

      bool doReadTransferLatest() override {
        return true;
      }


      bool doWriteTransfer(ChimeraTK::VersionNumber /*versionNumber*/={}) override {
        if(isReadOnly()) {
          throw DeviceException("Writing to constant-type registers of logical name mapping devices is not possible.",
              DeviceException::REGISTER_IS_READ_ONLY);
        }
        _info->value_int[0] = _fixedPointConverter.toRaw(NDRegisterAccessor<UserType>::buffer_2D[0][0]);
        return false;
      }

      bool mayReplaceOther(const boost::shared_ptr<TransferElement const> &) const override {
        return false;   // never replace, since it does not optimise anything
      }

      bool isReadOnly() const override {
        return _info->targetType == LNMBackendRegisterInfo::TargetType::INT_CONSTANT;
      }

      bool isReadable() const override {
        return true;
      }

      bool isWriteable() const override {
        return _info->targetType != LNMBackendRegisterInfo::TargetType::INT_CONSTANT;
      }

      void doPostRead() override {
        NDRegisterAccessor<UserType>::buffer_2D[0][0] =_fixedPointConverter.toCooked<UserType>(_info->value_int[0]);
      }

      AccessModeFlags getAccessModeFlags() const override {
        return {};
      }

    protected:

      /// register and module name
      RegisterPath _registerPathName;

      /// backend device
      boost::shared_ptr<LogicalNameMappingBackend> _dev;

      /// register information. We have a shared pointer to the original RegisterInfo inside the map, since
      /// we need to modify the value in it (in case of a writeable variable register)
      boost::shared_ptr<LNMBackendRegisterInfo> _info;

      /// fixed point converter to handle type conversions from our "raw" type int to the requested user type.
      /// Note: no actual fixed point conversion is done, it is just used for the type conversion!
      FixedPointConverter _fixedPointConverter;

      std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() override {
        return { boost::enable_shared_from_this<TransferElement>::shared_from_this() };
      }

      std::list< boost::shared_ptr<TransferElement> > getInternalElements() override {
        return {};
      }

      void replaceTransferElement(boost::shared_ptr<TransferElement> /*newElement*/) override {}  // LCOV_EXCL_LINE


  };

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(LNMBackendVariableAccessor);

}    // namespace ChimeraTK

#endif /* CHIMERA_TK_LNM_BACKEND_BUFFERING_VARIABLE_ACCESSOR_H */
