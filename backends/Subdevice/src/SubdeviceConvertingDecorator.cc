// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SubdeviceConvertingDecorator.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  FixedPointConvertingDecorator<UserType, TargetUserType>::FixedPointConvertingDecorator(
      const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<TargetUserType>>& target,
      NumericAddressedRegisterInfo const& registerInfo)
  : NDRegisterAccessorDecorator<UserType, TargetUserType>(target), _registerInfo(registerInfo) {
    assert(registerInfo.getNumberOfChannels() == 1);
    //_converterLoopHelper =
    //    RawConverter::ConverterLoopHelper::makeConverterLoopHelper<UserType>(registerInfo, 0, *this);
    RawConverter::detail::callWithConverterParamsFixedRaw<UserType, TargetUserType>(registerInfo, 0 /*channelIndex*/,
        [&]<typename RawType, RawConverter::SignificantBitsCase sc, RawConverter::FractionalCase fc, bool isSigned> {
          RawConverter::Converter<UserType, std::make_unsigned_t<RawType>, sc, fc, isSigned> converter(
              registerInfo.channels[0]);
          _converterLoopHelper =
              std::make_unique<RawConverter::ConverterLoopHelperImpl<UserType, std::make_unsigned_t<RawType>, sc, fc,
                  isSigned, decltype(*this)>>(registerInfo, 0 /*channelIndex*/, converter, *this);
        });
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  void FixedPointConvertingDecorator<UserType, TargetUserType>::doPreRead(TransferType type) {
    _target->preRead(type);
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  void FixedPointConvertingDecorator<UserType, TargetUserType>::doPostRead(TransferType type, bool hasNewData) {
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
  void FixedPointConvertingDecorator<UserType, TargetUserType>::doPostReadImpl(
      RawConverter::Converter<CookedType, RawType, sc, fc, isSigned> converter, [[maybe_unused]] size_t channelIndex) {
    for(auto [itsrc, itdst] = std::make_pair(_target->accessChannel(0).begin(), buffer_2D[0].begin());
        itdst != buffer_2D[0].end(); ++itsrc, ++itdst) {
      if constexpr(!std::is_same_v<RawType, ChimeraTK::Void>) {
        *itdst = converter.toCooked(*itsrc);
      }
    }
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  void FixedPointConvertingDecorator<UserType, TargetUserType>::doPreWrite(
      TransferType type, VersionNumber versionNumber) {
    _converterLoopHelper->doPreWrite();
    _target->setDataValidity(this->_dataValidity);
    _target->preWrite(type, versionNumber);
  }

  /********************************************************************************************************************/

  // Callback for ConverterLoopHelper, see documentation there.
  template<typename UserType, typename TargetUserType>
  template<class CookedType, typename RawType, RawConverter::SignificantBitsCase sc, RawConverter::FractionalCase fc,
      bool isSigned>
  void FixedPointConvertingDecorator<UserType, TargetUserType>::doPreWriteImpl(
      RawConverter::Converter<CookedType, RawType, sc, fc, isSigned> converter, [[maybe_unused]] size_t channelIndex) {
    for(auto [itsrc, itdst] = std::make_pair(buffer_2D[0].begin(), _target->accessChannel(0).begin());
        itsrc != buffer_2D[0].end(); ++itsrc, ++itdst) {
      if constexpr(!std::is_same_v<RawType, ChimeraTK::Void>) {
        *itdst = converter.toRaw(*itsrc);
      }
    }
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  void FixedPointConvertingDecorator<UserType, TargetUserType>::doPostWrite(
      TransferType type, VersionNumber versionNumber) {
    _target->postWrite(type, versionNumber);
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  [[nodiscard]] bool FixedPointConvertingDecorator<UserType, TargetUserType>::mayReplaceOther(
      const boost::shared_ptr<ChimeraTK::TransferElement const>& other) const {
    auto casted = boost::dynamic_pointer_cast<FixedPointConvertingDecorator<UserType, TargetUserType> const>(other);
    if(!casted) {
      return false;
    }
    if(_registerInfo != casted->_registerInfo) {
      return false;
    }
    return _target->mayReplaceOther(casted->_target);
  }

  /********************************************************************************************************************/
  INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(FixedPointConvertingDecorator, uint8_t);
  INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(FixedPointConvertingDecorator, uint16_t);
  INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(FixedPointConvertingDecorator, uint32_t);
  INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(FixedPointConvertingDecorator, uint64_t);
  // FIXME: get rid of the signed ints!
  // INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(FixedPointConvertingDecorator, int8_t);
  // INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(FixedPointConvertingDecorator, int16_t);
  INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(FixedPointConvertingDecorator, int32_t);
  // INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(FixedPointConvertingDecorator, int64_t);

} // namespace ChimeraTK
