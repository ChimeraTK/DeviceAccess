#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "Exception.h"
#include "MapFileParser.h"
#include "RegisterInfoMap.h"
#include "predicates.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  NumericAddressedRegisterInfo::NumericAddressedRegisterInfo(std::string const& name_, uint32_t nElements_,
      uint64_t address_, uint32_t nBytes_, uint64_t bar_, uint32_t width_, int32_t nFractionalBits_, bool signedFlag_,
      std::string const& module_, uint32_t nChannels_, bool is2DMultiplexed_, Access dataAccess_, Type dataType_,
      uint32_t interruptCtrlNumber_, uint32_t interruptNumber_)
  : name(name_), nElements(nElements_), nChannels(nChannels_), is2DMultiplexed(is2DMultiplexed_), address(address_),
    nBytes(nBytes_), bar(bar_), width(width_), nFractionalBits(nFractionalBits_), signedFlag(signedFlag_),
    module(module_), registerAccess(dataAccess_), dataType(dataType_), interruptCtrlNumber(interruptCtrlNumber_),
    interruptNumber(interruptNumber_) {
    if(nBytes_ > 0 && nElements_ > 0) {
      if(nBytes_ % nElements_ != 0) {
        // nBytes_ must be divisible by nElements_
        throw logic_error(
            "Number of bytes is not a multiple of number of elements for register " + name_ + ". Check your map file!");
      }
    }

    DataType rawDataInfo;
    if(nBytesPerElement() == 0 && !is2DMultiplexed_) {
      rawDataInfo = DataType::Void;
    }
    else if(nBytesPerElement() == 1 && !is2DMultiplexed_) {
      rawDataInfo = DataType::int8;
    }
    else if(nBytesPerElement() == 2 && !is2DMultiplexed_) {
      rawDataInfo = DataType::int16;
    }
    else if(nBytesPerElement() == 4 && !is2DMultiplexed_) {
      rawDataInfo = DataType::int32;
    }
    else {
      rawDataInfo = DataType::none;
    }

    if(dataType == IEEE754) {
      if(width == 32) {
        // Largest possible number +- 3e38, smallest possible number 1e-45
        // However, the actual precision is only 23+1 bit, which is < 1e9 relevant
        // digits. Hence, we don't have to add the 3e38 and the 1e45, but just add
        // the leading 0. comma and sign to the largest 45 digits
        dataDescriptor = DataDescriptor(DataDescriptor::FundamentalType::numeric, // fundamentalType
            false,                                                                // isIntegral
            true,                                                                 // isSigned
            3 + 45,                                                               // nDigits
            45,                                                                   // nFractionalDigits
            DataType::int32); // we have integer in the transport layer
      }
      else if(width == 64) {
        // smallest possible 5e-324, largest 2e308
        dataDescriptor = DataDescriptor(DataDescriptor::FundamentalType::numeric, // fundamentalType
            false,                                                                // isIntegral
            true,                                                                 // isSigned
            3 + 325,                                                              // nDigits
            325,                                                                  // nFractionalDigits
            DataType::int64);
      }
      else {
        throw logic_error("Wrong data width for data type IEEE754 for register " + name_ + ". Check your map file!");
      }
    }
    else if(dataType == FIXED_POINT) {
      if(width > 1) { // numeric type

        if(nFractionalBits_ > 0) {
          size_t nDigits =
              std::ceil(std::log10(std::pow(2, width_))) + (signedFlag_ ? 1 : 0) + (nFractionalBits_ != 0 ? 1 : 0);
          size_t nFractionalDigits = std::ceil(std::log10(std::pow(2, nFractionalBits_)));

          dataDescriptor = DataDescriptor(DataDescriptor::FundamentalType::numeric, // fundamentalType
              false,                                                                // isIntegral
              signedFlag_,                                                          // isSigned
              nDigits, nFractionalDigits, rawDataInfo);
        }
        else {
          size_t nDigits = std::ceil(std::log10(std::pow(2, width_ + nFractionalBits_))) + (signedFlag_ ? 1 : 0);

          dataDescriptor = DataDescriptor(DataDescriptor::FundamentalType::numeric, // fundamentalType
              true,                                                                 // isIntegral
              signedFlag_,                                                          // isSigned
              nDigits, 0, rawDataInfo);
        }
      }
      else if(width == 1) { // boolean
        dataDescriptor = DataDescriptor(DataDescriptor::FundamentalType::boolean, true, false, 1, 0, rawDataInfo);
      }
      else { // width == 0 -> nodata
        dataDescriptor = DataDescriptor(DataDescriptor::FundamentalType::nodata, false, false, 0, 0, rawDataInfo);
      }
    }
    else if(dataType == ASCII) {
      dataDescriptor = DataDescriptor(DataDescriptor::FundamentalType::string, false, false, 0, 0, rawDataInfo);
    }
    else if(dataType == VOID) {
      dataDescriptor = DataDescriptor(DataDescriptor::FundamentalType::nodata, false, false, 0, 0, rawDataInfo);
    }
  }

  /********************************************************************************************************************/

  const std::map<unsigned int, std::set<unsigned int>>& NumericAddressedRegisterCatalogue::getListOfInterrupts() const {
    return _mapOfInterrupts;
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
