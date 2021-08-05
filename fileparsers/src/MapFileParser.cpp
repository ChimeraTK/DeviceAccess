#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>

#include "Exception.h"
#include "MapFileParser.h"
#include "NumericAddressedBackendMuxedRegisterAccessor.h" // for the MULTIPLEXED_SEQUENCE_PREFIX constant

namespace ChimeraTK {

  RegisterInfoMapPointer MapFileParser::parse(const std::string& file_name) {
    std::ifstream file;
    std::string line;
    std::istringstream is;
    uint32_t line_nr = 0;

    file.open(file_name.c_str());
    if(!file) {
      throw ChimeraTK::logic_error("Cannot open file \"" + file_name + "\"");
    }
    RegisterInfoMapPointer pmap(new RegisterInfoMap(file_name));
    std::string name;        /**< Name of register */
    uint32_t nElements;      /**< Number of elements in register */
    uint64_t address;        /**< Relative address in bytes from beginning  of the
                                bar(Base Address Range)*/
    uint32_t nBytes;         /**< Size of register expressed in bytes */
    uint64_t bar;            /**< Number of bar with register */
    uint32_t width;          /**< Number of significant bits in the register */
    int32_t nFractionalBits; /**< Number of fractional bits */
    bool signedFlag;         /**< Signed/Unsigned flag */
    RegisterInfoMap::RegisterInfo::Access registerAccess;
    RegisterInfoMap::RegisterInfo::Type type;

    uint32_t interruptCtrlNo;
    uint32_t interruptNo;

    std::string module; /**< Name of the module this register is in*/

    while(std::getline(file, line)) {
      bool failed = false;
      line_nr++;
      // Remove whitespace from beginning of line
      line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](int c) { return !isspace(c); }));
      if(!line.size()) {
        continue;
      }
      if(line[0] == '#') {
        continue;
      }
      if(line[0] == '@') {
        std::string org_line = line;
        RegisterInfoMap::MetaData md;
        // Remove the '@' character...
        line.erase(line.begin(), line.begin() + 1);
        // ... and remove all the whitespace after it
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](int c) { return !isspace(c); }));
        is.str(line);
        is >> md.name;
        if(!is) {
          throw ChimeraTK::logic_error(
              "Parsing error in map file '" + file_name + "' on line " + std::to_string(line_nr));
        }
        line.erase(line.begin(), line.begin() + md.name.length());
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](int c) { return !isspace(c); }));
        md.value = line;
        pmap->insert(md);
        is.clear();
        continue;
      }
      is.str(line);

      std::string moduleAndRegisterName;
      is >> moduleAndRegisterName;

      std::pair<std::string, std::string> moduleAndNamePair = splitStringAtLastDot(moduleAndRegisterName);
      module = moduleAndNamePair.first;
      name = moduleAndNamePair.second;
      if(name.empty()) {
        throw ChimeraTK::logic_error("Parsing error in map file '" + file_name + "' on line " +
            std::to_string(line_nr) +
            ": "
            "empty register name");
      }

      is >> std::setbase(0) >> nElements >> std::setbase(0) >> address >> std::setbase(0) >> nBytes;
      if(!is) {
        throw ChimeraTK::logic_error(
            "Parsing error in map file '" + file_name + "' on line " + std::to_string(line_nr));
      }
      // first, set default values for 'optional' fields
      bar = 0x0;
      width = 32;
      nFractionalBits = 0;
      signedFlag = true;
      registerAccess = RegisterInfoMap::RegisterInfo::Access::READWRITE;
      type = RegisterInfoMap::RegisterInfo::Type::FIXED_POINT;
      interruptCtrlNo = 0;
      interruptNo = 0;

      is >> std::setbase(0) >> bar;
      if(is.fail()) {
        failed = true;
      }
      if(!failed) {
        is >> std::setbase(0) >> width;
        if(is.fail()) {
          failed = true;
        }
        else {
          if(width > 32) {
            throw ChimeraTK::logic_error("Parsing error in map file '" + file_name + "' on line " +
                std::to_string(line_nr) +
                ": "
                "register width too big");
          }
        }
      }
      if(!failed) {
        std::string bitInterpretation;
        is >> bitInterpretation;
        if(is.fail()) {
          failed = true;
        }
        else {
          // width is needed to determine whether type is VOID
          auto type_and_nFractionBits = getTypeAndNFractionalBits(bitInterpretation, width);
          type = type_and_nFractionBits.first;
          nFractionalBits = type_and_nFractionBits.second;

          if(nFractionalBits > 1023 || nFractionalBits < -1024) {
            throw ChimeraTK::logic_error("Parsing error in map file '" + file_name + "' on line " +
                std::to_string(line_nr) +
                ": "
                "too many fractional bits");
          }
        }
      }

      if(!failed) {
        is >> std::setbase(0) >> signedFlag;
        if(is.fail()) {
          failed = true;
        }
      }

      if(type == RegisterInfoMap::RegisterInfo::Type::VOID) std::cout << "VOID type detected for " << name << std::endl;

      if(!failed) {
        std::string accessString;
        is >> accessString;
        if(is.fail()) {
          failed = true;
        }
        else {
          //first transform to uppercase
          std::transform(accessString.begin(), accessString.end(), accessString.begin(),
              [](unsigned char c) { return std::toupper(c); });

          //first check if access mode is INTERRUPT and additionally check the interrupt controller number and interrupt number
          auto interruptData = checkIfInterruptAndGetInterruptCtrlNoAndInterruptNo(accessString);

          if(interruptData.first) {
            registerAccess = RegisterInfoMap::RegisterInfo::Access::INTERRUPT;
            //auto irCtrlNo_and_irNo = interruptData.second;
            interruptCtrlNo = interruptData.second.first;
            interruptNo = interruptData.second.second;
          }
          else if(accessString == "RO")
            registerAccess = RegisterInfoMap::RegisterInfo::Access::READ;
          else if(accessString == "RW")
            registerAccess = RegisterInfoMap::RegisterInfo::Access::READWRITE;
          else if(accessString == "WO")
            registerAccess = RegisterInfoMap::RegisterInfo::Access::WRITE;
          else
            throw ChimeraTK::logic_error("Parsing error in map file '" + file_name + "' on line " +
                std::to_string(line_nr) +
                ": "
                "invalid data access");
        }
      }
      is.clear();

      checkFileConsitencyAndThrowIfError(
          registerAccess, type, nElements, address, nBytes, bar, width, nFractionalBits, signedFlag);

      RegisterInfoMap::RegisterInfo registerInfo(name, nElements, address, nBytes, bar, width, nFractionalBits,
          signedFlag, module, 1, false, registerAccess, type, interruptCtrlNo, interruptNo);
      pmap->insert(registerInfo);
    }

    // search for 2D registers and add 2D entries
    std::vector<RegisterInfoMap::RegisterInfo> newInfos;
    for(auto& info : *pmap) {
      // check if 2D register, otherwise ignore
      if(info.name.substr(0, MULTIPLEXED_SEQUENCE_PREFIX.length()) != MULTIPLEXED_SEQUENCE_PREFIX) continue;
      // name of the 2D register is the name without the sequence prefix
      name = info.name.substr(MULTIPLEXED_SEQUENCE_PREFIX.length());
      // count number of channels and number of entries per channel
      size_t nChannels = 0;
      size_t nBytesPerEntry = 0; // nb. of bytes per entry for all channels together
      // We have to aggregate the fractional/ signed information of all cannels.
      // Afterwards we set fractional to 9999 (way out of range, max allowed is
      // 1023) if there are fractional bits, just to indicate that the register is
      // not integer and probably a floating point accessor should be used (e.g.
      // in QtHardMon).
      bool isSigned = false;
      bool isInteger = true;
      uint32_t maxWidth = 0;
      auto cat = pmap->getRegisterCatalogue();
      while(cat.hasRegister(RegisterPath(info.module) / (SEQUENCE_PREFIX + name + "_" + std::to_string(nChannels)))) {
        RegisterInfoMap::RegisterInfo subInfo;
        pmap->getRegisterInfo(
            RegisterPath(info.module) / (SEQUENCE_PREFIX + name + "_" + std::to_string(nChannels)), subInfo);
        nBytesPerEntry += subInfo.nBytes;
        nChannels++;
        if(subInfo.signedFlag) {
          isSigned = true;
        }
        if(subInfo.nFractionalBits > 0) {
          isInteger = false;
        }
        maxWidth = std::max(maxWidth, subInfo.width);
      }
      if(nChannels == 0) continue;
      // Compute number of elements. Note that there may be additional padding bytes specified in the map file. The
      // integer division is then rounding down, so it may be that nElements * nBytesPerEntry != info.nBytes.
      nElements = info.nBytes / nBytesPerEntry;
      // add it to the map
      RegisterInfoMap::RegisterInfo newEntry(name, nElements, info.address, nElements * nBytesPerEntry, info.bar,
          maxWidth, (isInteger ? 0 : 9999) /*fractional bits*/, isSigned, info.module, nChannels, true,
          info.registerAccess);
      newInfos.push_back(newEntry);
    }
    // insert the new entries to the catalogue
    for(auto& entry : newInfos) {
      pmap->insert(entry);
    }

    return pmap;
  }

  std::pair<std::string, std::string> MapFileParser::splitStringAtLastDot(std::string moduleDotName) {
    size_t lastDotPosition = moduleDotName.rfind('.');

    // some special case handlings to avoid string::split from throwing exceptions
    if(lastDotPosition == std::string::npos) {
      // no dot found, the whole string is the second argument
      return std::make_pair(std::string(), moduleDotName);
    }

    if(lastDotPosition == 0) {
      // the first character is a dot, everything from pos 1 is the second
      // argument
      if(moduleDotName.size() == 1) {
        // it's just a dot, return  two empty strings
        return std::make_pair(std::string(), std::string());
      }

      // split after the first character
      return std::make_pair(std::string(), moduleDotName.substr(1));
    }

    if(lastDotPosition == (moduleDotName.size() - 1)) {
      // the last character is a dot. The second argument is empty
      return std::make_pair(moduleDotName.substr(0, lastDotPosition), std::string());
    }

    return std::make_pair(moduleDotName.substr(0, lastDotPosition), moduleDotName.substr(lastDotPosition + 1));
  }

  std::pair<RegisterInfoMap::RegisterInfo::Type, int> MapFileParser::getTypeAndNFractionalBits(
      std::string bitInterpretation, unsigned int width) {
    if(width == 0) return {RegisterInfoMap::RegisterInfo::Type::VOID, 0};
    if(bitInterpretation == "IEEE754") return {RegisterInfoMap::RegisterInfo::Type::IEEE754, 0};
    if(bitInterpretation == "ASCII") return {RegisterInfoMap::RegisterInfo::Type::ASCII, 0};

    // If it is a digit the implicit interpretation is FixedPoint
    try {
      int nBits = std::stoi(bitInterpretation, nullptr,
          0); // base 0 = auto, hex or dec or oct
      return {RegisterInfoMap::RegisterInfo::Type::FIXED_POINT, nBits};
    }
    catch(std::exception& e) {
      throw ChimeraTK::logic_error(std::string("Map file error in bitInterpretation: wrong argument '") +
          bitInterpretation + "', caught exception: " + e.what());
    }
  }

  std::pair<bool, std::pair<unsigned int, unsigned int>> MapFileParser::
      checkIfInterruptAndGetInterruptCtrlNoAndInterruptNo(std::string accessTypeStr) {
    std::string strToFind("INTERRUPT");
    auto pos = accessTypeStr.find(strToFind);
    if(pos == std::string::npos) return {false, {0, 0}};

    unsigned int intCtrlNo = 0;
    unsigned int intNo = 0;
    accessTypeStr.erase(pos, strToFind.length());

    auto delimiterPos = accessTypeStr.find(":");
    if(delimiterPos != std::string::npos) {
      std::string intCtrlNoStr = accessTypeStr.substr(0, delimiterPos);
      std::string intNoStr = accessTypeStr.substr(delimiterPos + 1);
      try {
        intCtrlNo = std::stoul(intCtrlNoStr, nullptr, 0); // base 0 = auto, hex or dec or oct
      }
      catch(std::exception& e) {
        throw ChimeraTK::logic_error(
            std::string("Map file error in accessString: wrong argument in interrupt controller number. Argument: '") +
            intCtrlNoStr + "', caught exception: " + e.what());
      }

      try {
        intNo = std::stoul(intNoStr, nullptr, 0); // base 0 = auto, hex or dec or oct
      }
      catch(std::exception& e) {
        throw ChimeraTK::logic_error(
            std::string("Map file error in accessString: wrong argument in interrupt number. Argument: '") + intNoStr +
            "', caught exception: " + e.what());
      }
    }
    else
      throw ChimeraTK::logic_error(
          std::string("Map file error in accessString: Delimiter ':' not found in INTERRPUT description "));

    //return {true, {intCtrlNo, intNo}};
    return std::make_pair(true, std::make_pair(intCtrlNo, intNo));
  }

  void MapFileParser::checkFileConsitencyAndThrowIfError(RegisterInfoMap::RegisterInfo::Access registerAccessMode,
      RegisterInfoMap::RegisterInfo::Type registerType, uint32_t nElements, uint64_t address, uint32_t nBytes,
      uint64_t bar, uint32_t width, int32_t nFractionalBits, bool signedFlag) {
    // if type is VOID, access mode must be interrupt (cannot be READWRITE, READ or WRITE)
    if(registerType == RegisterInfoMap::RegisterInfo::Type::VOID &&
        registerAccessMode != RegisterInfoMap::RegisterInfo::Access::INTERRUPT)
      throw ChimeraTK::logic_error(
          std::string("Map file error. Register Type is VOID and access mode is different than INTERRUPT. "));
    // if register type is VOID then all fields must be '0'
    if(registerType == RegisterInfoMap::RegisterInfo::Type::VOID) {
      if(width)
        throw ChimeraTK::logic_error(
            std::string("Map file error. Register Type is VOID and number of width is " + std::to_string(width)));
      if(nElements)
        throw ChimeraTK::logic_error(std::string(
            "Map file error. Register Type is VOID and number of elements is " + std::to_string(nElements)));
      if(address)
        throw ChimeraTK::logic_error(
            std::string("Map file error. Register Type is VOID and address is " + std::to_string(address)));
      if(nBytes)
        throw ChimeraTK::logic_error(
            std::string("Map file error. Register Type is VOID and number of bytes is " + std::to_string(nBytes)));
      if(bar)
        throw ChimeraTK::logic_error(
            std::string("Map file error. Register Type is VOID and bar is " + std::to_string(bar)));
      if(nFractionalBits)
        throw ChimeraTK::logic_error(std::string(
            "Map file error. Register Type is VOID and fractional position is " + std::to_string(nFractionalBits)));
      if(signedFlag)
        throw ChimeraTK::logic_error(std::string("Map file error. Register Type is VOID and signed is true"));
    }
  }

} // namespace ChimeraTK
