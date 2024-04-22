// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "DeviceBackendImpl.h"
#include "NumericAddressedRegisterCatalogue.h"

#include <mutex>
#include <string>

namespace ChimeraTK {
  class SubdeviceRegisterAccessor;

  /**
   *  Backend for subdevices which are passed through some register or area of
   * another device (subsequently called target device). The subdevice is close to
   * a numeric addressed backend and has a map file of the same format (but BARs
   * other than 0 are not supported). The target device may be of any type.
   *
   *  The sdm URI syntax for setting up the subdevice depends on the protocol used
   * to pass through the registers. The following pass-trough types are supported:
   *
   *  - "area" type:  use a 1D register as an address space.\n
   *  URI scheme:\n
   *  \verbatim(subdevice?type=area&device=<targetDevice>&area=<targetRegister>&map=mapFile>)\endverbatim
   *
   *  - "3regs" type: use three scalar registers: address, data and status. Before
   * access, a value of 0 in the status register is awaited. Next, the address is
   * written to the address register. The value is then written to resp. read from
   * the data register.\n
   * URI scheme:\n
   * \verbatim(subdevice?type=3regs&device=<targetDevice>&address=<addressRegister>&data=<dataRegister>&status=<statusRegister>&sleep=<usecs>&map=<mapFile>)\endverbatim
   * The sleep parameter is optional and defaults to 100 usecs. It sets the polling interval for the status register in
   * microseconds. Another optional parameter "dataDelay" can be used to configure an additional delay in microseconds
   * between the write of the address and the data registers (defaults to 0 usecs).
   *
   *  - "2regs" type: same as "3regs" but without a status register. Instead the
   * sleep parameter is mandatory and specifies the fixed sleep time before each
   * operation.
   *  - "areaHandshake" type: mapped area, but before write operations to registers
   *    inside the map, waits for value 0 in the status register like in 3regs mode.
   *    The sleep parameter is optional.\n
   * URI scheme:\n
   * \verbatim(subdevice?type=areaHandshake&device=<targetDevice>&area=<targetRegister>&map=mapFile&status=<statusRegister>&sleep=<usecs>)\endverbatim
   *
   *  Example: We like to use the register "APP.0.EXT_PZ16M" of the device with
   * the alias name "TCK7_0" in our dmap file as a target and the file
   * piezo_pz16m_acc1_r0.mapp as a map file. The file contains addresses relative
   * to the beginning of the register "APP.0.EXT_PZ16M". The URI then looks like
   * this:
   *  \verbatim(subdevice?type=area&device=TCK7_0&area=APP.0.EXT_PZ16M&map=piezo_pz16m_acc1_r0.mapp)\endverbatim
   *
   *  @warning The protocol for the types "3regs" and "2regs" is not yet
   * finalised. In particular read transfers might change in future. Please do not
   * use these for reading in production code!
   */
  class SubdeviceBackend : public DeviceBackendImpl {
   public:
    explicit SubdeviceBackend(std::map<std::string, std::string> parameters);

    void open() override;

    void close() override;

    std::string readDeviceInfo() override {
      return "Subdevice"; /// @todo extend information
    }

    static boost::shared_ptr<DeviceBackend> createInstance(
        std::string address, std::map<std::string, std::string> parameters);

    RegisterCatalogue getRegisterCatalogue() const override;

    MetadataCatalogue getMetadataCatalogue() const override;

   protected:
    friend class SubdeviceRegisterAccessor;

    enum class Type {
      area,           //< address space is visible as an area in the target device
      threeRegisters, //< use three registers (address, data and status) in target
                      //< device. status must be 0 when idle
      twoRegisters,   //< same as three registers but without status
      areaHandshake   //< address space visible as an area in the target device, and wait on status 0
    };

    /// Mutex to deal with concurrent access to the device
    std::mutex mutex;

    /// type of the subdeivce
    Type type;

    /// timeout (in milliseconds), used in threeRegisters to throw a runtime_error if status register stuck at 1
    size_t timeout{10000};

    /// the target device name
    std::string targetAlias;

    /// The target device backend itself. We are using directly a backend since we want to obtain
    /// NDRegisterAccessors which we can directly return in  getRegisterAccessor_impl().
    boost::shared_ptr<ChimeraTK::DeviceBackend> targetDevice;

    /// for type == area: the name of the target register
    std::string targetArea;

    /// for type == threeRegisters or twoRegisters: the name of the target registers
    std::string targetAddress, targetData, targetControl;

    /// for type == threeRegisters or twoRegisters: sleep time of polling loop resp. between operations, in usecs.
    size_t sleepTime{100};

    /// for type == threeRegisters or twoRegisters: sleep time between address and data write
    size_t addressToDataDelay{0};

    /// map from register names to addresses
    NumericAddressedRegisterCatalogue _registerMap;
    MetadataCatalogue _metadataCatalogue;

    /// Check consistency of the passed sizes and offsets against the information in the map file
    /// Will adjust numberOfWords to the default value if 0
    static void verifyRegisterAccessorSize(const NumericAddressedRegisterInfo& info, size_t& numberOfWords,
        size_t wordOffsetInRegister, bool enforceAlignment);

    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getRegisterAccessor_impl(
        const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);
    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(SubdeviceBackend, getRegisterAccessor_impl, 4);

    /// getRegisterAccessor implemenation for area types
    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getRegisterAccessor_area(
        const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);

    /// getRegisterAccessor implemenation for threeRegisters types
    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getRegisterAccessor_synchronized(
        const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister,
        const AccessModeFlags& flags);

    /// obtain the target backend if not yet done
    void obtainTargetBackend();

    void setExceptionImpl() noexcept override;

    void activateAsyncRead() noexcept override;

    bool needAreaParam() { return type == Type::area || type == Type::areaHandshake; }
    bool needStatusParam() { return type == Type::threeRegisters || type == Type::areaHandshake; }

    // helper for reducing code duplication among template specializations
    boost::shared_ptr<SubdeviceRegisterAccessor> getRegisterAccessor_helper(const NumericAddressedRegisterInfo& info,
        size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);
  };

} // namespace ChimeraTK
