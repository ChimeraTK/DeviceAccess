/**
 *      @brief          Provides storage object for registers description taken
 * from MAP file
 */
#pragma once

#include <boost/shared_ptr.hpp>
#include <iostream>
#include <list>
#include <cstdint>
#include <string>
#include <vector>

#include "BackendRegisterCatalogue.h"
#include "BackendRegisterInfoBase.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  class NumericAddressedRegisterInfo : public BackendRegisterInfoBase {
   public:
    enum Access { READ = 1 << 0, WRITE = 1 << 1, READWRITE = READ | WRITE, INTERRUPT = 1 << 2 };

    /** 
     *  Enum descibing the data interpretation:
     *  \li Fixed point (includes integer = 0 fractional bits)
     *  \li IEEE754 floating point
     *  \li ASCII ascii characters
     *  \li VOID no data content, just trigger events (push type) FIXME: Currently implicit by 0 bits width
     */
    enum Type { FIXED_POINT, IEEE754, ASCII, VOID };

    /// Constructor to set all data members. They all have default values, so
    /// this also acts as default constructor.
    explicit NumericAddressedRegisterInfo(std::string const& name_ = std::string(), // an empty string
        uint32_t nElements_ = 0, uint64_t address_ = 0, uint32_t nBytes_ = 0, uint64_t bar_ = 0, uint32_t width_ = 32,
        int32_t nFractionalBits_ = 0, bool signedFlag_ = true, std::string const& module_ = std::string(),
        uint32_t nChannels_ = 1, bool is2DMultiplexed_ = false, Access dataAccess_ = Access::READWRITE,
        Type dataType_ = Type::FIXED_POINT, uint32_t interruptCtrlNumber_ = 0, uint32_t interruptNumber_ = 0);

    NumericAddressedRegisterInfo(const NumericAddressedRegisterInfo&) = default;

    NumericAddressedRegisterInfo& operator=(const NumericAddressedRegisterInfo& other) = default;

    [[nodiscard]] RegisterPath getRegisterName() const override {
      RegisterPath path = RegisterPath(module) / name;
      path.setAltSeparator(".");
      return path;
    }

    [[nodiscard]] unsigned int getNumberOfElements() const override { return nElements; }

    [[nodiscard]] unsigned int getNumberOfChannels() const override {
      if(is2DMultiplexed) return nChannels;
      return 1;
    }

    [[nodiscard]] unsigned int getNumberOfDimensions() const override {
      if(is2DMultiplexed) return 2;
      if(nElements > 1) return 1;
      return 0;
    }

    [[nodiscard]] const DataDescriptor& getDataDescriptor() const override { return dataDescriptor; }

    [[nodiscard]] bool isReadable() const override {
      return ((registerAccess & Access::READ) | (registerAccess & Access::INTERRUPT)) != 0;
    }

    [[nodiscard]] bool isWriteable() const override { return (registerAccess & Access::WRITE) != 0; }

    [[nodiscard]] AccessModeFlags getSupportedAccessModes() const override {
      if(registerAccess == Access::INTERRUPT && ((dataType == Type::VOID) || (is2DMultiplexed))) {
        return {AccessMode::wait_for_new_data};
      }
      if(registerAccess == Access::INTERRUPT) {
        return {AccessMode::raw, AccessMode::wait_for_new_data};
      }
      if(is2DMultiplexed) {
        return {};
      }
      return {AccessMode::raw};
    }

    std::string name;        /**< Name of register */
    uint32_t nElements;      /**< Number of elements in register */
    uint32_t nChannels;      /**< Number of channels/sequences */
    bool is2DMultiplexed;    /**< Flag if register is a 2D multiplexed
                                        register (otherwise it is 1D or scalar) */
    uint64_t address;        /**< Relative address in bytes from beginning  of
                                        the bar(Base Address Range)*/
    uint32_t nBytes;         /**< Size of register expressed in bytes */
    uint64_t bar;            /**< Number of bar with register */
    uint32_t width;          /**< Number of significant bits in the register */
    int32_t nFractionalBits; /**< Number of fractional bits */
    bool signedFlag;         /**< Signed/Unsigned flag */
    std::string module;      /**< Name of the module this register is in*/
    Access registerAccess;   /**< Data access direction: Read, write, read
                                        and write or interrupt */
    Type dataType;           /**< Data type (fixpoint, floating point)*/

    uint32_t interruptCtrlNumber;
    uint32_t interruptNumber;

    [[nodiscard]] uint32_t nBytesPerElement() const { return nElements > 0 ? nBytes / nElements : 0; }

    DataDescriptor dataDescriptor;

    [[nodiscard]] std::unique_ptr<BackendRegisterInfoBase> clone() const override {
      auto* info = new NumericAddressedRegisterInfo(*this);
      return std::unique_ptr<BackendRegisterInfoBase>(info);
    }
  };

  /********************************************************************************************************************/

  class NumericAddressedRegisterCatalogue : public BackendRegisterCatalogue<NumericAddressedRegisterInfo> {
   public:
    [[nodiscard]] const std::map<unsigned int, std::set<unsigned int>>& getListOfInterrupts() const;

    void addRegister(NumericAddressedRegisterInfo registerInfo) {
      if(registerInfo.registerAccess == NumericAddressedRegisterInfo::Access::INTERRUPT) {
        _mapOfInterrupts[registerInfo.interruptCtrlNumber].insert(registerInfo.interruptNumber);
      }
      BackendRegisterCatalogue<NumericAddressedRegisterInfo>::addRegister(registerInfo);
    }

   protected:
    /** 
     *  Map of interrupts. Key is an interrupt controller number and value is a set of interrupts numbers assigned to
     *  the given interrupt controller.
     */
    std::map<unsigned int, std::set<unsigned int>> _mapOfInterrupts;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
