#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "Exception.h"
#include "MapFileParser.h"
#include "RegisterInfoMap.h"
#include "predicates.h"

namespace ChimeraTK {

  std::ostream& operator<<(std::ostream& os, const RegisterInfoMap& registerInfoMap) {
    os << "=======================================" << std::endl;
    os << "MAP FILE NAME: " << registerInfoMap._mapFileName << std::endl;
    os << "---------------------------------------" << std::endl;
    for(auto it = registerInfoMap._catalogue.metadata_begin(); it != registerInfoMap._catalogue.metadata_end(); ++it) {
      os << "METADATA-> NAME: \"" << it->first << "\" VALUE: " << it->second << std::endl;
    }
    os << "---------------------------------------" << std::endl;
    for(auto it = registerInfoMap.begin(); it != registerInfoMap.end(); ++it) {
      os << *(it) << std::endl;
    }
    os << "=======================================";
    return os;
  }

  void RegisterInfoMap::insert(RegisterInfo& elem) {
    _catalogue.addRegister(boost::shared_ptr<RegisterInfo>(new RegisterInfo(elem)));
  }

  void RegisterInfoMap::insert(MetaData& elem) { _catalogue.addMetadata(elem.name, elem.value); }

  void RegisterInfoMap::getRegisterInfo(
      const std::string& reg_name, RegisterInfoMap::RegisterInfo& value, const std::string& reg_module) const {
    auto info = _catalogue.getRegister(RegisterPath(reg_module) / reg_name);
    auto infoCast = boost::static_pointer_cast<RegisterInfoMap::RegisterInfo>(info);
    value = *infoCast;
  }

  void RegisterInfoMap::getRegisterInfo(int reg_nr, RegisterInfo& value) const {
    int count = 0;
    for(auto it = _catalogue.begin(); it != _catalogue.end(); ++it) {
      if(count == reg_nr) {
        auto infoCast = boost::static_pointer_cast<const RegisterInfoMap::RegisterInfo>(it.get());
        value = *infoCast;
        return;
      }
      count++;
    }
    throw ChimeraTK::logic_error("Cannot find register in map file");
  }

  void RegisterInfoMap::getMetaData(const std::string& metaDataName, std::string& metaDataValue) const {
    metaDataValue = _catalogue.getMetadata(metaDataName);
  }

  namespace {

    typedef struct _addresses {
      uint64_t start;
      uint64_t end;
      std::vector<RegisterInfoMap::RegisterInfo>::iterator iter;
    } addresses;

  } // namespace

  bool RegisterInfoMap::check(ErrorList& err, RegisterInfoMap::ErrorList::ErrorElem::TYPE level) {
    std::vector<addresses> v_addresses;
    std::vector<addresses>::iterator v_iter;
    addresses address;

    std::vector<RegisterInfo> map_file;
    for(auto it = _catalogue.begin(); it != _catalogue.end(); ++it) {
      auto infoCast = boost::static_pointer_cast<RegisterInfoMap::RegisterInfo>(it.get());
      map_file.push_back(*infoCast);
    }

    std::vector<RegisterInfo>::iterator iter_p, iter_n;
    bool ret = true;

    err.clear();
    if(map_file.size() < 2) return true;
    std::sort(map_file.begin(), map_file.end(), compareRegisterInfosByName_functor());
    iter_p = map_file.begin();
    iter_n = iter_p + 1;
    while(1) {
      if((iter_p->name == iter_n->name) && (iter_p->module == iter_n->module)) {
        err.insert(ErrorList::ErrorElem(ErrorList::ErrorElem::ERROR, ErrorList::ErrorElem::NONUNIQUE_REGISTER_NAME,
            *iter_p, *iter_n, _mapFileName));
        ret = false;
      }
      iter_p = iter_n;
      iter_n = ++iter_n;
      if(iter_n == map_file.end()) {
        break;
      }
    }

    for(iter_p = map_file.begin(); iter_p != map_file.end(); ++iter_p) {
      address.start = iter_p->address;
      address.end = iter_p->address + iter_p->nBytes;
      address.iter = iter_p;
      for(v_iter = v_addresses.begin(); v_iter != v_addresses.end(); ++v_iter) {
        // only addresses in the same module are considered to overlap
        if(iter_p->module != v_iter->iter->module) {
          continue;
        }

        if(iter_p->bar == (v_iter->iter)->bar) {
          if((v_iter->start >= iter_p->address) && (v_iter->start < (iter_p->address + iter_p->nBytes))) {
            if(level >= ErrorList::ErrorElem::WARNING) {
              err.insert(ErrorList::ErrorElem(ErrorList::ErrorElem::WARNING,
                  ErrorList::ErrorElem::WRONG_REGISTER_ADDRESSES, *iter_p, *(v_iter->iter), _mapFileName));
              ret = false;
            }
          }
          else if((v_iter->start <= iter_p->address) && (iter_p->address < v_iter->end)) {
            if(level >= ErrorList::ErrorElem::WARNING) {
              err.insert(ErrorList::ErrorElem(ErrorList::ErrorElem::WARNING,
                  ErrorList::ErrorElem::WRONG_REGISTER_ADDRESSES, *iter_p, *(v_iter->iter), _mapFileName));
              ret = false;
            }
          }
        }
      }
      v_addresses.push_back(address);
    }
    return ret;
  }

  std::ostream& operator<<(std::ostream& os, const RegisterInfoMap::MetaData& me) {
    os << "METADATA-> NAME: \"" << me.name << "\" VALUE: " << me.value << std::endl;
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const RegisterInfoMap::RegisterInfo::Type& type) {
    switch(type) {
      case RegisterInfoMap::RegisterInfo::Type::FIXED_POINT:
        os << "FIXED_POINT";
        break;
      case RegisterInfoMap::RegisterInfo::Type::IEEE754:
        os << "IEEE754";
        break;
      case RegisterInfoMap::RegisterInfo::Type::ASCII:
        os << "ASCII";
        break;
      case RegisterInfoMap::RegisterInfo::Type::VOID:
        os << "VOID";
        break;
      default:
        os << "UNKNOWN";
    }
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const RegisterInfoMap::RegisterInfo::Access& access) {
    switch(access) {
      case RegisterInfoMap::RegisterInfo::Access::READ:
        os << "RO";
        break;
      case RegisterInfoMap::RegisterInfo::Access::WRITE:
        os << "WO";
        break;
      case RegisterInfoMap::RegisterInfo::Access::READWRITE:
        os << "RW";
        break;
      case RegisterInfoMap::RegisterInfo::Access::INTERRUPT:
        os << "INTERRUPT";
        break;
      default:
        os << "UNKNOWN";
    }
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const RegisterInfoMap::RegisterInfo& registerInfo) {
    os << registerInfo.name << " 0x" << std::hex << registerInfo.nElements << " 0x" << registerInfo.address << " 0x"
       << registerInfo.nBytes << " 0x" << registerInfo.bar << std::dec << " " << registerInfo.width << " "
       << registerInfo.nFractionalBits << " " << (registerInfo.signedFlag ? "true" : "false")
       << (!registerInfo.module.empty() ? " " + registerInfo.module : " <noModule>") << " "
       << registerInfo.registerAccess << " " << registerInfo.dataType << " " << registerInfo.interruptCtrlNumber << " "
       << registerInfo.interruptNumber; // << " " << registerInfo.getNumberOfDimensions();
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const RegisterInfoMap::ErrorList::ErrorElem::TYPE& me) {
    switch(me) {
      case RegisterInfoMap::ErrorList::ErrorElem::ERROR:
        os << "ERROR";
        break;
      case RegisterInfoMap::ErrorList::ErrorElem::WARNING:
        os << "WARNING";
        break;
      default:
        os << "UNKNOWN";
        break;
    }
    return os;
  }

  RegisterInfoMap::ErrorList::ErrorElem::ErrorElem(RegisterInfoMap::ErrorList::ErrorElem::TYPE info_type,
      RegisterInfoMap::ErrorList::ErrorElem::MAP_FILE_ERR e_type, const RegisterInfoMap::RegisterInfo& reg_1,
      const RegisterInfoMap::RegisterInfo& reg_2, const std::string& file_name) {
    _errorType = e_type;
    _errorRegister1 = reg_1;
    _errorRegister2 = reg_2;
    _errorFileName = file_name;
    _type = info_type;
  }

  std::ostream& operator<<(std::ostream& os, const RegisterInfoMap::ErrorList::ErrorElem& me) {
    switch(me._errorType) {
      case RegisterInfoMap::ErrorList::ErrorElem::NONUNIQUE_REGISTER_NAME:
        os << me._type << ": Found two registers with the same name: \"" << me._errorRegister1.name << "\" in file "
           << me._errorFileName;
        break;
      case RegisterInfoMap::ErrorList::ErrorElem::WRONG_REGISTER_ADDRESSES:
        os << me._type << ": Found two registers with overlapping addresses: \"" << me._errorRegister1.name
           << "\" and \"" << me._errorRegister2.name << "\" in file " << me._errorFileName;
        break;
    }
    return os;
  }

  void RegisterInfoMap::ErrorList::clear() { errors.clear(); }

  void RegisterInfoMap::ErrorList::insert(const RegisterInfoMap::ErrorList::ErrorElem& elem) { errors.push_back(elem); }

  std::ostream& operator<<(std::ostream& os, const RegisterInfoMap::ErrorList& me) {
    std::list<RegisterInfoMap::ErrorList::ErrorElem>::const_iterator iter;
    for(iter = me.errors.begin(); iter != me.errors.end(); ++iter) {
      os << *iter << std::endl;
    }
    return os;
  }

  RegisterInfoMap::RegisterInfoMap() {}

  RegisterInfoMap::RegisterInfoMap(const std::string& file_name) : _mapFileName(file_name) {}

  const std::string& RegisterInfoMap::getMapFileName() const { return _mapFileName; }

  size_t RegisterInfoMap::getMapFileSize() const { return _catalogue.getNumberOfRegisters(); }

  RegisterInfoMap::iterator RegisterInfoMap::begin() { return {_catalogue.begin()}; }

  RegisterInfoMap::const_iterator RegisterInfoMap::begin() const { return {_catalogue.begin()}; }

  RegisterInfoMap::iterator RegisterInfoMap::end() { return {_catalogue.end()}; }

  RegisterInfoMap::const_iterator RegisterInfoMap::end() const { return {_catalogue.end()}; }

  std::list<RegisterInfoMap::RegisterInfo> RegisterInfoMap::getRegistersInModule(std::string const& moduleName) {
    // first sort all elements accordind the names (module first, then register in
    // module) make a copy to keep the original order from the map file
    std::vector<RegisterInfo> sortedRegisterInfos;
    for(auto it = _catalogue.begin(); it != _catalogue.end(); ++it) {
      auto infoCast = boost::static_pointer_cast<RegisterInfoMap::RegisterInfo>(it.get());
      sortedRegisterInfos.push_back(*infoCast);
    }
    std::sort(sortedRegisterInfos.begin(), sortedRegisterInfos.end(), compareRegisterInfosByName_functor());

    // The vector is sorted, first module, than register name.
    // Find the first iterator with the module name, starting at the beginning,
    // then search for the first iterator with another name, starting from the
    // previousy found first match.
    std::vector<RegisterInfo>::iterator firstMatchingIterator =
        std::find_if(sortedRegisterInfos.begin(), sortedRegisterInfos.end(), compareModuleName_pred(moduleName));
    std::vector<RegisterInfo>::iterator firstNotMatchingIterator =
        std::find_if(firstMatchingIterator, sortedRegisterInfos.end(), std::not1(compareModuleName_pred(moduleName)));

    // fill the list
    std::list<RegisterInfoMap::RegisterInfo> registerInfoList;
    for(std::vector<RegisterInfo>::iterator it = firstMatchingIterator; it != firstNotMatchingIterator; ++it) {
      registerInfoList.push_back(*it);
    }

    return registerInfoList;
  }

  RegisterInfoMap::MetaData::MetaData(std::string const& name_, std::string const& value_)
  : name(name_), value(value_) {}

  RegisterInfoMap::RegisterInfo::RegisterInfo(std::string const& name_, uint32_t nElements_, uint64_t address_,
      uint32_t nBytes_, uint64_t bar_, uint32_t width_, int32_t nFractionalBits_, bool signedFlag_,
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
    if(nBytesPerElement() == 1 && !is2DMultiplexed_) {
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
        dataDescriptor = DataDescriptor(RegisterInfo::FundamentalType::numeric, // fundamentalType
            false,                                                              // isIntegral
            true,                                                               // isSigned
            3 + 45,                                                             // nDigits
            45,                                                                 // nFractionalDigits
            DataType::int32); // we have integer in the transport layer
      }
      else if(width == 64) {
        // smallest possible 5e-324, largest 2e308
        dataDescriptor = DataDescriptor(RegisterInfo::FundamentalType::numeric, // fundamentalType
            false,                                                              // isIntegral
            true,                                                               // isSigned
            3 + 325,                                                            // nDigits
            325,                                                                // nFractionalDigits
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

          dataDescriptor = DataDescriptor(RegisterInfo::FundamentalType::numeric, // fundamentalType
              false,                                                              // isIntegral
              signedFlag_,                                                        // isSigned
              nDigits, nFractionalDigits, rawDataInfo);
        }
        else {
          size_t nDigits = std::ceil(std::log10(std::pow(2, width_ + nFractionalBits_))) + (signedFlag_ ? 1 : 0);

          dataDescriptor = DataDescriptor(RegisterInfo::FundamentalType::numeric, // fundamentalType
              true,                                                               // isIntegral
              signedFlag_,                                                        // isSigned
              nDigits, 0, rawDataInfo);
        }
      }
      else if(width == 1) { // boolean
        dataDescriptor = DataDescriptor(RegisterInfo::FundamentalType::boolean, true, false, 1, 0, rawDataInfo);
      }
      else { // width == 0 -> nodata
        dataDescriptor = DataDescriptor(RegisterInfo::FundamentalType::nodata, false, false, 0, 0, rawDataInfo);
      }
    }
    else if(dataType == ASCII) {
      dataDescriptor = DataDescriptor(RegisterInfo::FundamentalType::string, false, false, 0, 0, rawDataInfo);
    }
    else if(dataType == VOID) {
      dataDescriptor = DataDescriptor(RegisterInfo::FundamentalType::nodata, false, false, 0, 0, rawDataInfo);
    }
  }
  const RegisterCatalogue& RegisterInfoMap::getRegisterCatalogue() { return _catalogue; }

} // namespace ChimeraTK
