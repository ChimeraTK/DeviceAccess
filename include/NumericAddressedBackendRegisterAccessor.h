// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "ForwardDeclarations.h"
#include "NDRegisterAccessor.h"
#include "NumericAddressedLowLevelTransferElement.h"
#include "RawConverter.h"

#include <ChimeraTK/cppext/finally.hpp>

namespace ChimeraTK {

  template<typename UserType, bool isRaw>
  class NumericAddressedBackendRegisterAccessor;

  /********************************************************************************************************************/

  /**
   * Implementation of the NDRegisterAccessor for NumericAddressedBackends for scalar and 1D registers.
   */
  template<typename UserType, bool isRaw>
  class NumericAddressedBackendRegisterAccessor : public NDRegisterAccessor<UserType> {
   public:
    NumericAddressedBackendRegisterAccessor(const boost::shared_ptr<DeviceBackend>& dev,
        const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);

    void doReadTransferSynchronously() override;

    bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber) override;

    void doPostRead(TransferType type, bool hasNewData) override;

    template<class UserType2, typename RawType, RawConverter::SignificantBitsCase sc, RawConverter::FractionalCase fc,
        bool isSigned>
    void doPostReadImpl(
        RawConverter::Converter<UserType2, RawType, sc, fc, isSigned> converter, [[maybe_unused]] size_t channelIndex);

    void doPreWrite(TransferType type, VersionNumber versionNumber) override;

    template<class UserType2, typename RawType, RawConverter::SignificantBitsCase sc, RawConverter::FractionalCase fc,
        bool isSigned>
    void doPreWriteImpl(
        RawConverter::Converter<UserType2, RawType, sc, fc, isSigned> converter, [[maybe_unused]] size_t channelIndex);

    void doPreRead(TransferType type) override;

    void doPostWrite(TransferType type, VersionNumber versionNumber) override;

    [[nodiscard]] bool mayReplaceOther(const boost::shared_ptr<TransferElement const>& other) const override;

    [[nodiscard]] bool isReadOnly() const override;

    [[nodiscard]] bool isReadable() const override;

    [[nodiscard]] bool isWriteable() const override;

    template<typename COOKED_TYPE>
    COOKED_TYPE getAsCooked_impl(unsigned int channel, unsigned int sample);

    template<typename COOKED_TYPE>
    void setAsCooked_impl(unsigned int channel, unsigned int sample, COOKED_TYPE value);

    // a local typename so the DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER does
    // not get confused by the comma which separates the two template parameters
    using THIS_TYPE = NumericAddressedBackendRegisterAccessor<UserType, isRaw>;

    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(THIS_TYPE, getAsCooked_impl, 2);
    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(THIS_TYPE, setAsCooked_impl, 3);

    void setExceptionBackend(boost::shared_ptr<DeviceBackend> exceptionBackend) override;

   protected:
    /** Address, size and fixed-point representation information of the register
     * from the map file */
    NumericAddressedRegisterInfo _registerInfo;

    /** Converter to interpret the data */
    std::unique_ptr<RawConverter::ConverterLoopHelper> _converterLoopHelper;

    /** raw accessor */
    boost::shared_ptr<NumericAddressedLowLevelTransferElement> _rawAccessor;

    /** the backend to use for the actual hardware access */
    boost::shared_ptr<NumericAddressedBackend> _dev;

    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override;

    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override;

    void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) override;

    using NDRegisterAccessor<UserType>::buffer_2D;
  }; // namespace ChimeraTK

  /********************************************************************************************************************/

  DECLARE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(NumericAddressedBackendRegisterAccessor, true);
  DECLARE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(NumericAddressedBackendRegisterAccessor, false);

  /********************************************************************************************************************/

} // namespace ChimeraTK
