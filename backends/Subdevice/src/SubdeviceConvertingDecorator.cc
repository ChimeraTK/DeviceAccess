// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SubdeviceConvertingDecorator.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  ConvertingDecorator<UserType, TargetUserType>::ConvertingDecorator(
      const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<TargetUserType>>& target,
      NumericAddressedRegisterInfo const& registerInfo)
  : NDRegisterAccessorDecorator<UserType, TargetUserType>(target), _registerInfo(registerInfo) {
    assert(registerInfo.getNumberOfChannels() == 1);
    _converterLoopHelper = RawConverter::ConverterLoopHelper::makeConverterLoopHelperFixedRaw<UserType, TargetUserType>(
        registerInfo, 0 /*channel*/, 0 /* implParameter*/, *this);
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  void ConvertingDecorator<UserType, TargetUserType>::doPreRead(TransferType type) {
    _target->preRead(type);
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  void ConvertingDecorator<UserType, TargetUserType>::doPostRead(TransferType type, bool hasNewData) {
    _target->postRead(type, hasNewData);
    if(!hasNewData) {
      return;
    }

    _converterLoopHelper->doPostRead();

    this->_dataValidity = _target->dataValidity();
    this->_versionNumber = _target->getVersionNumber();
  }

  /********************************************************************************************************************/

  // Callback for ConverterLoopHelper, see documentation there.
  template<typename UserType, typename TargetUserType>
  template<class CookedType, typename RawType, RawConverter::SignificantBitsCase sc, RawConverter::FractionalCase fc,
      bool isSigned>
  void ConvertingDecorator<UserType, TargetUserType>::doPostReadImpl(
      RawConverter::Converter<CookedType, RawType, sc, fc, isSigned> converter, [[maybe_unused]] size_t implParameter) {
    for(auto [itsrc, itdst] = std::make_pair(_target->accessChannel(0).begin(), buffer_2D[0].begin());
        itdst != buffer_2D[0].end(); ++itsrc, ++itdst) {
      if constexpr(!std::is_same_v<RawType, ChimeraTK::Void>) {
        *itdst = converter.toCooked(*itsrc);
      }
    }
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  void ConvertingDecorator<UserType, TargetUserType>::doPreWrite(TransferType type, VersionNumber versionNumber) {
    _converterLoopHelper->doPreWrite();
    _target->setDataValidity(this->_dataValidity);
    _target->preWrite(type, versionNumber);
  }

  /********************************************************************************************************************/

  // Callback for ConverterLoopHelper, see documentation there.
  template<typename UserType, typename TargetUserType>
  template<class CookedType, typename RawType, RawConverter::SignificantBitsCase sc, RawConverter::FractionalCase fc,
      bool isSigned>
  void ConvertingDecorator<UserType, TargetUserType>::doPreWriteImpl(
      RawConverter::Converter<CookedType, RawType, sc, fc, isSigned> converter, [[maybe_unused]] size_t implParameter) {
    for(auto [itsrc, itdst] = std::make_pair(buffer_2D[0].begin(), _target->accessChannel(0).begin());
        itsrc != buffer_2D[0].end(); ++itsrc, ++itdst) {
      if constexpr(!std::is_same_v<RawType, ChimeraTK::Void>) {
        *itdst = converter.toRaw(*itsrc);
      }
    }
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  void ConvertingDecorator<UserType, TargetUserType>::doPostWrite(TransferType type, VersionNumber versionNumber) {
    _target->postWrite(type, versionNumber);
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  [[nodiscard]] bool ConvertingDecorator<UserType, TargetUserType>::mayReplaceOther(
      const boost::shared_ptr<ChimeraTK::TransferElement const>& other) const {
    auto casted = boost::dynamic_pointer_cast<ConvertingDecorator<UserType, TargetUserType> const>(other);
    if(!casted) {
      return false;
    }
    if(_registerInfo != casted->_registerInfo) {
      return false;
    }
    return _target->mayReplaceOther(casted->_target);
  }

  /********************************************************************************************************************/
  INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(ConvertingDecorator, uint8_t);
  INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(ConvertingDecorator, uint16_t);
  INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(ConvertingDecorator, uint32_t);
  INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(ConvertingDecorator, uint64_t);

  // FIXME: get rid of the signed ints!
  INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(ConvertingDecorator, int32_t);

} // namespace ChimeraTK
