// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "ForwardDeclarations.h"
#include "NDRegisterAccessor.h"
#include "NumericAddressedLowLevelTransferElement.h"
#include "RawConverter.h"
#include "ReadCoordinator.h"

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

    // Callback for ConverterLoopHelper, see documentation there.
    template<class CookedType, typename RawType, RawConverter::SignificantBitsCase sc, RawConverter::FractionalCase fc,
        bool isSigned>
    void doPostReadImpl(RawConverter::Converter<CookedType, RawType, sc, fc, isSigned> converter,
        [[maybe_unused]] size_t implParameter);

    void doPreWrite(TransferType type, VersionNumber versionNumber) override;

    // Callback for ConverterLoopHelper, see documentation there.
    template<class CookedType, typename RawType, RawConverter::SignificantBitsCase sc, RawConverter::FractionalCase fc,
        bool isSigned>
    void doPreWriteImpl(RawConverter::Converter<CookedType, RawType, sc, fc, isSigned> converter,
        [[maybe_unused]] size_t implParameter);

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

    /**
     * Get the read consumer's registration with the read coordinator of its raw element. The gate result of this
     * consumer (see Registration::gateClosed()) can therefore be read from outside, e.g. by a GatedByDecorator.
     */
    [[nodiscard]] detail::ReadCoordinator::Registration<UserType>& readRegistration() { return _readRegistration; }

   protected:
    /** Address, size and fixed-point representation information of the register
     * from the map file */
    NumericAddressedRegisterInfo _registerInfo;

    /** Converter to interpret the data */
    std::unique_ptr<RawConverter::ConverterLoopHelper> _converterLoopHelper;

    /** raw accessor */
    boost::shared_ptr<NumericAddressedLowLevelTransferElement> _rawAccessor;

    /**
     * The expected value of this consumer's gating register, which opens the gate. Only meaningful when the register
     * carries a gatedBy (see _registerInfo). The gating register accessor itself is owned by the low-level element's
     * gate group, keyed by the gating register path; this accessor does not hold it.
     */
    uint64_t _gateValue{0};

    /**
     * Registration of this read consumer with the read coordinator of its raw element. The handle owns the staging
     * buffer the coordinator fills.
     */
    detail::ReadCoordinator::Registration<UserType> _readRegistration;

    /** the backend to use for the actual hardware access */
    boost::shared_ptr<NumericAddressedBackend> _dev;

    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override;

    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override;

    void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) override;

    /** (Re-)register this read consumer with the read coordinator of the current raw element. */
    void registerWithReadCoordinator();

    using NDRegisterAccessor<UserType>::buffer_2D;
  }; // namespace ChimeraTK

  /********************************************************************************************************************/

  DECLARE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(NumericAddressedBackendRegisterAccessor, true);
  DECLARE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(NumericAddressedBackendRegisterAccessor, false);

  /********************************************************************************************************************/

} // namespace ChimeraTK
