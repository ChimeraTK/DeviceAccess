// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "NDRegisterAccessorDecorator.h"
#include "RawConverter.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  template<typename UserType, typename TargetUserType>
  class FixedPointConvertingDecorator : public NDRegisterAccessorDecorator<UserType, TargetUserType> {
   public:
    explicit FixedPointConvertingDecorator(
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

    void doPreRead(TransferType type) override { _target->preRead(type); }

    void doPostRead(TransferType type, bool hasNewData) override {
      _target->postRead(type, hasNewData);
      if(!hasNewData) {
        return;
      }

      _converterLoopHelper->doPostRead();

      this->_dataValidity = _target->dataValidity();
      this->_versionNumber = _target->getVersionNumber();
    }

    // Callback for ConverterLoopHelper, see documentation there.
    template<class CookedType, typename RawType, RawConverter::SignificantBitsCase sc, RawConverter::FractionalCase fc,
        bool isSigned>
    void doPostReadImpl(RawConverter::Converter<CookedType, RawType, sc, fc, isSigned> converter,
        [[maybe_unused]] size_t channelIndex) {
      for(auto [itsrc, itdst] = std::make_pair(_target->accessChannel(0).begin(), buffer_2D[0].begin());
          itdst != buffer_2D[0].end(); ++itsrc, ++itdst) {
        if constexpr(!std::is_same_v<RawType, ChimeraTK::Void>) {
          *itdst = converter.toCooked(*itsrc);
        }
      }
    }

    void doPreWrite(TransferType type, VersionNumber versionNumber) override {
      _converterLoopHelper->doPreWrite();
      _target->setDataValidity(this->_dataValidity);
      _target->preWrite(type, versionNumber);
    }

    // Callback for ConverterLoopHelper, see documentation there.
    template<class CookedType, typename RawType, RawConverter::SignificantBitsCase sc, RawConverter::FractionalCase fc,
        bool isSigned>
    void doPreWriteImpl(RawConverter::Converter<CookedType, RawType, sc, fc, isSigned> converter,
        [[maybe_unused]] size_t channelIndex) {
      for(auto [itsrc, itdst] = std::make_pair(buffer_2D[0].begin(), _target->accessChannel(0).begin());
          itsrc != buffer_2D[0].end(); ++itsrc, ++itdst) {
        if constexpr(!std::is_same_v<RawType, ChimeraTK::Void>) {
          *itdst = converter.toRaw(*itsrc);
        }
      }
    }

    void doPostWrite(TransferType type, VersionNumber versionNumber) override {
      _target->postWrite(type, versionNumber);
    }

    [[nodiscard]] bool mayReplaceOther(
        const boost::shared_ptr<ChimeraTK::TransferElement const>& other) const override {
      auto casted = boost::dynamic_pointer_cast<FixedPointConvertingDecorator<UserType, TargetUserType> const>(other);
      if(!casted) {
        return false;
      }
      if(_registerInfo != casted->_registerInfo) {
        return false;
      }
      return _target->mayReplaceOther(casted->_target);
    }

   protected:
    std::unique_ptr<RawConverter::ConverterLoopHelper> _converterLoopHelper;
    NumericAddressedRegisterInfo _registerInfo;

    using NDRegisterAccessorDecorator<UserType, TargetUserType>::_target;
    using NDRegisterAccessor<UserType>::buffer_2D;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
