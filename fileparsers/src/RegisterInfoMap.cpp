#include <algorithm>
#include <stdexcept>

#include "RegisterInfoMap.h"
#include "predicates.h"
#include "MapException.h"
#include "MapFileParser.h"

namespace mtca4u {

  std::ostream& operator<<(std::ostream &os, const RegisterInfoMap& registerInfoMap) {
    size_t size;
    os << "=======================================" << std::endl;
    os << "MAP FILE NAME: " << registerInfoMap._mapFileName << std::endl;
    os << "---------------------------------------" << std::endl;
    for (size = 0; size < registerInfoMap._metadata.size(); size++) {
      os << registerInfoMap._metadata[size];
    }
    os << "---------------------------------------" << std::endl;
    for (size = 0; size < registerInfoMap._mapFileElements.size(); size++) {
      os << registerInfoMap._mapFileElements[size] << std::endl;
    }
    os << "=======================================";
    return os;
  }

  void RegisterInfoMap::insert(RegisterInfo &elem) {
    _mapFileElements.push_back(elem);
  }

  void RegisterInfoMap::insert(MetaData &elem) {
    _metadata.push_back(elem);
  }

  void RegisterInfoMap::getRegisterInfo(const std::string& reg_name, RegisterInfo &value, const std::string& reg_module) const{

    std::string mergedName = ( reg_module.length() > 0 ? reg_module + "." + reg_name : reg_name );
    auto moduleAndRegister = MapFileParser::splitStringAtLastDot(mergedName);

    std::vector<RegisterInfo>::const_iterator iter;
    iter = std::find_if(_mapFileElements.begin(), _mapFileElements.end(),
        findRegisterByName_pred(moduleAndRegister.second, moduleAndRegister.first));
    if (iter == _mapFileElements.end()) {
      throw MapFileException("Cannot find register " + reg_module + (reg_module.empty()?"":".") + reg_name +
          " in map file: " + _mapFileName, LibMapException::EX_NO_REGISTER_IN_MAP_FILE);
    }
    value = *iter;
  }

  void RegisterInfoMap::getRegisterInfo(int reg_nr, RegisterInfo &value) const {
    try {
      value = _mapFileElements.at(reg_nr);
    } catch (std::out_of_range &) {
      throw MapFileException("Cannot find register in map file", LibMapException::EX_NO_REGISTER_IN_MAP_FILE);
    }
  }

  void RegisterInfoMap::getMetaData(const std::string &metaDataName, std::string& metaDataValue) const{
    std::vector<RegisterInfoMap::MetaData>::const_iterator iter;

    iter = std::find_if(_metadata.begin(), _metadata.end(), findMetaDataByName_pred(metaDataName));
    if (iter == _metadata.end()) {
      throw MapFileException("Cannot find metadata " + metaDataName + " in map file: " + _mapFileName, LibMapException::EX_NO_METADATA_IN_MAP_FILE);
    }
    metaDataValue = (*iter).value;
  }

  namespace {

    typedef struct _addresses {
        uint32_t start;
        uint32_t end;
        std::vector<RegisterInfoMap::RegisterInfo>::iterator iter;
    } addresses;

  }

  bool RegisterInfoMap::check(ErrorList &err, RegisterInfoMap::ErrorList::ErrorElem::TYPE level) {
    std::vector<addresses> v_addresses;
    std::vector<addresses>::iterator v_iter;
    addresses address;

    std::vector<RegisterInfo> map_file = _mapFileElements;
    std::vector<RegisterInfo>::iterator iter_p, iter_n;
    bool ret = true;

    err.clear();
    if (_mapFileElements.size() < 2)
      return true;
    std::sort(map_file.begin(), map_file.end(), compareRegisterInfosByName_functor());
    iter_p = map_file.begin();
    iter_n = iter_p + 1;
    while (1) {
      if ( (iter_p->name == iter_n->name) && (iter_p->module == iter_n->module) ) {
        err.insert(ErrorList::ErrorElem(ErrorList::ErrorElem::ERROR,
            ErrorList::ErrorElem::NONUNIQUE_REGISTER_NAME,
            *iter_p, *iter_n, _mapFileName));
        ret = false;
      }
      iter_p = iter_n;
      iter_n = ++iter_n;
      if (iter_n == map_file.end()) {
        break;
      }
    }

    for (iter_p = map_file.begin(); iter_p != map_file.end(); ++iter_p) {
      address.start = iter_p->address;
      address.end = iter_p->address + iter_p->nBytes;
      address.iter = iter_p;
      for (v_iter = v_addresses.begin(); v_iter != v_addresses.end(); ++v_iter) {
        // only addresses in the same module are considered to overlap
        if (iter_p->module != v_iter->iter->module){
          continue;
        }

        if (iter_p->bar == (v_iter->iter)->bar) {
          if ( (v_iter->start >= iter_p->address) &&
              (v_iter->start < (iter_p->address + iter_p->nBytes)) ) {
            if (level >= ErrorList::ErrorElem::WARNING){
              err.insert(ErrorList::ErrorElem(ErrorList::ErrorElem::WARNING,
                  ErrorList::ErrorElem::WRONG_REGISTER_ADDRESSES,
                  *iter_p, *(v_iter->iter), _mapFileName));
              ret = false;
            }
          } else if ( (v_iter->start <= iter_p->address) && (iter_p->address < v_iter->end) ) {
            if (level >= ErrorList::ErrorElem::WARNING){
              err.insert(ErrorList::ErrorElem(ErrorList::ErrorElem::WARNING,
                  ErrorList::ErrorElem::WRONG_REGISTER_ADDRESSES,
                  *iter_p, *(v_iter->iter), _mapFileName));
              ret = false;
            }
          }
        }
      }
      v_addresses.push_back(address);
    }
    return ret;
  }

  std::ostream& operator<<(std::ostream &os, const RegisterInfoMap::MetaData& me) {
    os << "METADATA-> NAME: \"" << me.name << "\" VALUE: " << me.value << std::endl;
    return os;
  }

  std::ostream& operator<<(std::ostream &os, const RegisterInfoMap::RegisterInfo& registerInfo) {
    os << registerInfo.name << " 0x" << std::hex << registerInfo.nElements << " 0x" << registerInfo.address
        << " 0x" << registerInfo.nBytes << " 0x" << registerInfo.bar << std::dec
        << " " << registerInfo.width << " " << registerInfo.nFractionalBits << " "
        << (registerInfo.signedFlag?"true":"false" )
        << (!registerInfo.module.empty()?" "+registerInfo.module:"");
    return os;
  }

  std::ostream& operator<<(std::ostream &os, const RegisterInfoMap::ErrorList::ErrorElem::TYPE& me) {
    switch (me) {
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

  RegisterInfoMap::ErrorList::ErrorElem::ErrorElem(RegisterInfoMap::ErrorList::ErrorElem::TYPE info_type, RegisterInfoMap::ErrorList::ErrorElem::MAP_FILE_ERR e_type, const RegisterInfoMap::RegisterInfo &reg_1, const RegisterInfoMap::RegisterInfo &reg_2, const std::string &file_name) {
    _errorType = e_type;
    _errorRegister1 = reg_1;
    _errorRegister2 = reg_2;
    _errorFileName = file_name;
    _type = info_type;
  }

  std::ostream& operator<<(std::ostream &os, const RegisterInfoMap::ErrorList::ErrorElem& me) {
    switch (me._errorType) {
      case RegisterInfoMap::ErrorList::ErrorElem::NONUNIQUE_REGISTER_NAME:
        os << me._type << ": Found two registers with the same name: \"" << me._errorRegister1.name << "\" in file " << me._errorFileName << " in lines " << me._errorRegister1.lineNumber << " and " << me._errorRegister2.lineNumber;
        break;
      case RegisterInfoMap::ErrorList::ErrorElem::WRONG_REGISTER_ADDRESSES:
        os << me._type << ": Found two registers with overlapping addresses: \"" << me._errorRegister1.name << "\" and \"" << me._errorRegister2.name << "\" in file " << me._errorFileName << " in lines " << me._errorRegister1.lineNumber << " and " << me._errorRegister2.lineNumber;
        break;
    }
    return os;
  }

  void RegisterInfoMap::ErrorList::clear() {
    errors.clear();
  }

  void RegisterInfoMap::ErrorList::insert(const RegisterInfoMap::ErrorList::ErrorElem& elem) {
    errors.push_back(elem);
  }

  std::ostream& operator<<(std::ostream &os, const RegisterInfoMap::ErrorList& me) {
    std::list<RegisterInfoMap::ErrorList::ErrorElem>::const_iterator iter;
    for (iter = me.errors.begin(); iter != me.errors.end(); ++iter) {
      os << *iter << std::endl;
    }
    return os;
  }

  RegisterInfoMap::RegisterInfoMap()
  {

  }

  RegisterInfoMap::RegisterInfoMap(const std::string &file_name)
  : _mapFileName(file_name) {
  }

  const std::string& RegisterInfoMap::getMapFileName() const {
    return _mapFileName;
  }

  size_t RegisterInfoMap::getMapFileSize() const {
    return _mapFileElements.size();
  }

  RegisterInfoMap::iterator RegisterInfoMap::begin() {
    return _mapFileElements.begin();
  }

  RegisterInfoMap::const_iterator RegisterInfoMap::begin() const{
    return _mapFileElements.begin();
  }

  RegisterInfoMap::const_iterator RegisterInfoMap::end() const{
    return _mapFileElements.end();
  }

  RegisterInfoMap::iterator RegisterInfoMap::end(){
    return _mapFileElements.end();
  }

  std::list< RegisterInfoMap::RegisterInfo > RegisterInfoMap::getRegistersInModule( std::string const & moduleName){
    // first sort all elements accordind the names (module first, then register in module)
    // make a copy to keep the original order from the map file
    std::vector<RegisterInfo> sortedRegisterInfos = _mapFileElements;
    std::sort(sortedRegisterInfos.begin(), sortedRegisterInfos.end(), compareRegisterInfosByName_functor());

    // The vector is sorted, first module, than register name.
    // Find the first iterator with the module name, starting at the beginning,
    // then search for the first iterator with another name, starting from the previousy found first match.
    std::vector<RegisterInfo>::iterator firstMatchingIterator =
        std::find_if( sortedRegisterInfos.begin(), sortedRegisterInfos.end(), compareModuleName_pred( moduleName ) );
    std::vector<RegisterInfo>::iterator firstNotMatchingIterator =
        std::find_if( firstMatchingIterator, sortedRegisterInfos.end(),
            std::not1( compareModuleName_pred(moduleName) ) );

    // fill the list
    std::list<RegisterInfoMap::RegisterInfo> registerInfoList;
    for (std::vector<RegisterInfo>::iterator it = firstMatchingIterator;
        it != firstNotMatchingIterator; ++it){
      registerInfoList.push_back( *it );
    }

    return registerInfoList;
  }

  RegisterInfoMap::MetaData::MetaData( std::string const & name_,
      std::string const & value_)
  : name(name_), value(value_)
  {}

  RegisterInfoMap::RegisterInfo::RegisterInfo( std::string const & name_,
      uint32_t nElements_,
      uint32_t address_,
      uint32_t nBytes_,
      uint32_t bar_,
      uint32_t width_,
      int32_t  nFractionalBits_,
      bool     signedFlag_,
      uint32_t lineNumber_,
      std::string const & module_)
  : name( name_ ),  nElements(nElements_), address(address_),
    nBytes(nBytes_), bar(bar_), width(width_), nFractionalBits(nFractionalBits_),
    signedFlag(signedFlag_), lineNumber(lineNumber_), module(module_)
  {}

}//namespace mtca4u
