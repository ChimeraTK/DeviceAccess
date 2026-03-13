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
        NumericAddressedRegisterInfo const& registerInfo);

    void doPreRead(TransferType type) override;

    void doPostRead(TransferType type, bool hasNewData) override;

    // Callback for ConverterLoopHelper, see documentation there.
    template<class CookedType, typename RawType, RawConverter::SignificantBitsCase sc, RawConverter::FractionalCase fc,
        bool isSigned>
    void doPostReadImpl(
        RawConverter::Converter<CookedType, RawType, sc, fc, isSigned> converter, [[maybe_unused]] size_t channelIndex);

    void doPreWrite(TransferType type, VersionNumber versionNumber) override;

    // Callback for ConverterLoopHelper, see documentation there.
    template<class CookedType, typename RawType, RawConverter::SignificantBitsCase sc, RawConverter::FractionalCase fc,
        bool isSigned>
    void doPreWriteImpl(
        RawConverter::Converter<CookedType, RawType, sc, fc, isSigned> converter, [[maybe_unused]] size_t channelIndex);

    void doPostWrite(TransferType type, VersionNumber versionNumber) override;

    [[nodiscard]] bool mayReplaceOther(const boost::shared_ptr<ChimeraTK::TransferElement const>& other) const override;

   protected:
    std::unique_ptr<RawConverter::ConverterLoopHelper> _converterLoopHelper;
    NumericAddressedRegisterInfo _registerInfo;

    using NDRegisterAccessorDecorator<UserType, TargetUserType>::_target;
    using NDRegisterAccessor<UserType>::buffer_2D;
  };

  /********************************************************************************************************************/

  DECLARE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(FixedPointConvertingDecorator, uint8_t);
  DECLARE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(FixedPointConvertingDecorator, uint16_t);
  DECLARE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(FixedPointConvertingDecorator, uint32_t);
  DECLARE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(FixedPointConvertingDecorator, uint64_t);

  // FIXME: get rid of the signed ints!
  DECLARE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(FixedPointConvertingDecorator, int32_t);

} // namespace ChimeraTK
