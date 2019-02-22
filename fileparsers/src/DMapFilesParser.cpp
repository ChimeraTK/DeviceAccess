#include <algorithm>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <sys/types.h>

#include "DMapFilesParser.h"
#include "Exception.h"
#include "RegisterInfoMap.h"
#include "Utilities.h"
#include "parserUtilities.h"
#include "predicates.h"

namespace utilities = ChimeraTK::parserUtilities;

namespace ChimeraTK {

  void DMapFilesParser::parse_dirs(const std::vector<std::string>& dirs) {
    std::vector<std::string>::const_iterator iter;
    cleanAll();
    for(iter = dirs.begin(); iter != dirs.end(); ++iter) {
      parse_one_directory(*iter);
    }
  }

  void DMapFilesParser::parse_dir(const std::string& dir) {
    cleanAll();
    parse_one_directory(dir);
  }

  void DMapFilesParser::parse_file(const std::string& fileName) {
    DeviceInfoMapPointer dmap;
    std::vector<DeviceInfoMap::DeviceInfo>::iterator dmap_elem_iter;
    std::vector<RegisterInfoMapPointer>::iterator map_file_iter;
    RegisterInfoMapPointer map;
    std::string absolutePathToDMapDir = utilities::convertToAbsolutePath(utilities::extractDirectory(fileName));
    cleanAll();
    dmap = _dmapFileParser.parse(fileName);
    for(dmap_elem_iter = dmap->_deviceInfoElements.begin(); dmap_elem_iter != dmap->_deviceInfoElements.end();
        ++dmap_elem_iter) {
      std::string absPathToCurrentMapFile =
          utilities::concatenatePaths(absolutePathToDMapDir, dmap_elem_iter->mapFileName);
      map_file_iter = std::find_if(_mapFiles.begin(), _mapFiles.end(), findMapFileByName_pred(absPathToCurrentMapFile));
      if(map_file_iter == _mapFiles.end()) {
        map = _mapFileParser.parse(absPathToCurrentMapFile);
        _mapFiles.push_back(map);
      }
      else {
        map = *map_file_iter;
      }
      _dmapElements.push_back(std::make_pair(*dmap_elem_iter, map));
    }
  }

  void DMapFilesParser::parse_one_directory(const std::string& dir) {
    DIR* dp;
    struct dirent* dirp;
    size_t found;
    std::string file_name;
    DeviceInfoMapPointer dmap;
    std::vector<DeviceInfoMap::DeviceInfo>::iterator dmap_elem_iter;
    std::vector<RegisterInfoMapPointer>::iterator map_file_iter;
    RegisterInfoMapPointer map;
    std::string dir_new = dir;
    if(dir[dir.length() - 1] != '/') dir_new += "/";

    if((dp = opendir(dir.c_str())) == NULL) {
      throw ChimeraTK::logic_error("Cannot open directory: \"" + dir + "\"");
    }

    while((dirp = readdir(dp)) != NULL) {
      if(dirp->d_type != DT_REG) {
        if(dirp->d_type != DT_UNKNOWN) continue;
      }
      file_name = std::string(dirp->d_name);
      found = file_name.find_last_of(".");
      if(found == std::string::npos) continue;
      if(file_name.substr(found) == ".dmap") {
        try {
          dmap = _dmapFileParser.parse(dir_new + file_name);
        }
        catch(ChimeraTK::detail::EmptyDMapFileException&) {
          continue;
        }
        catch(ChimeraTK::logic_error&) {
          closedir(dp);
          throw;
        }
        catch(ChimeraTK::runtime_error&) {
          closedir(dp);
          throw;
        }
        /* fixme? repetition from parse_file structure this better?*/
        for(dmap_elem_iter = dmap->_deviceInfoElements.begin(); dmap_elem_iter != dmap->_deviceInfoElements.end();
            ++dmap_elem_iter) {
          map_file_iter =
              std::find_if(_mapFiles.begin(), _mapFiles.end(), findMapFileByName_pred((*dmap_elem_iter).mapFileName));
          if(map_file_iter == _mapFiles.end()) {
            try {
              if((*dmap_elem_iter).mapFileName[0] == '.') {
                map = _mapFileParser.parse(dir_new + (*dmap_elem_iter).mapFileName);
              }
              else {
                map = _mapFileParser.parse((*dmap_elem_iter).mapFileName);
              }
            }
            catch(ChimeraTK::logic_error&) {
              closedir(dp);
              throw;
            }
            catch(ChimeraTK::runtime_error&) {
              closedir(dp);
              throw;
            }
            _mapFiles.push_back(map);
          }
          else {
            map = *map_file_iter;
          }
          _dmapElements.push_back(std::make_pair(*dmap_elem_iter, map));
        }
      }
    }
    closedir(dp);
    if(_dmapElements.empty()) {
      // TODO: change message? No dmap files in dir
      throw ChimeraTK::logic_error("DMAP file is empty or does not exist");
    }
  }

  // FIXME: Why is dlevel not used?
  bool DMapFilesParser::check(DeviceInfoMap::ErrorList::ErrorElem::TYPE /*dlevel*/,
      RegisterInfoMap::ErrorList::ErrorElem::TYPE mlevel, DeviceInfoMap::ErrorList& dmap_err,
      RegisterInfoMap::ErrorList& map_err) {
    std::vector<std::pair<DeviceInfoMap::DeviceInfo, RegisterInfoMapPointer>> dmaps = _dmapElements;
    std::vector<std::pair<DeviceInfoMap::DeviceInfo, RegisterInfoMapPointer>>::iterator iter_p, iter_n;
    bool ret = true;

    dmap_err.clear();
    map_err.clear();
    if(dmaps.size() < 2) {
      return true;
    }

    /* FIXME: why not use dmapFile.check instead of repeating here */
    std::sort(dmaps.begin(), dmaps.end(), copmaredRegisterInfosByName_functor());
    iter_p = dmaps.begin();
    iter_n = iter_p + 1;
    while(1) {
      if((*iter_p).first.deviceName == (*iter_n).first.deviceName) {
        if((*iter_p).first.uri != (*iter_n).first.uri || (*iter_p).first.mapFileName != (*iter_n).first.mapFileName) {
          dmap_err.insert(DeviceInfoMap::ErrorList::ErrorElem(DeviceInfoMap::ErrorList::ErrorElem::ERROR,
              DeviceInfoMap::ErrorList::ErrorElem::NONUNIQUE_DEVICE_NAME, (*iter_p).first, (*iter_n).first));
          ret = false;
        }
      }
      iter_p = iter_n;
      iter_n = ++iter_n;
      if(iter_n == dmaps.end()) {
        break;
      }
    }
    /* Why not pass map_err by reference? because we are clearing stuff inside
     * check.. why???
     * */
    RegisterInfoMap::ErrorList mapErr;
    std::vector<RegisterInfoMapPointer>::iterator map_iter;
    for(map_iter = _mapFiles.begin(); map_iter != _mapFiles.end(); ++map_iter) {
      if(!(*map_iter)->check(mapErr, mlevel)) {
        map_err.errors.splice(map_err.errors.end(), mapErr.errors);
        ret = false;
      }
    }

    return ret;
  }

  RegisterInfoMapPointer DMapFilesParser::getMapFile(const std::string& dev_name) {
    std::vector<std::pair<DeviceInfoMap::DeviceInfo, RegisterInfoMapPointer>>::iterator dmap_iter;
    dmap_iter = std::find_if(_dmapElements.begin(), _dmapElements.end(), findDevInPairByName_pred(dev_name));
    if(dmap_iter == _dmapElements.end()) {
      throw ChimeraTK::logic_error("Cannot find device " + dev_name);
    }
    return (*dmap_iter).second;
  }

  void DMapFilesParser::getdMapFileElem(const std::string& devName, DeviceInfoMap::DeviceInfo& dMapFileElem) {
    dMapFileElem = getdMapFileElem(devName);
  }

  DeviceInfoMap::DeviceInfo const& DMapFilesParser::getdMapFileElem(const std::string& devName) {
    std::vector<std::pair<DeviceInfoMap::DeviceInfo, RegisterInfoMapPointer>>::iterator dmap_iter;
    dmap_iter = std::find_if(_dmapElements.begin(), _dmapElements.end(), findDevInPairByName_pred(devName));
    if(dmap_iter == _dmapElements.end()) {
      throw ChimeraTK::logic_error("Cannot find device " + devName);
    }
    return (*dmap_iter).first;
  }

  void DMapFilesParser::getdMapFileElem(int elem_nr, DeviceInfoMap::DeviceInfo& dMapFileElem) {
    try {
      dMapFileElem = _dmapElements.at(elem_nr).first;
    }
    catch(std::out_of_range&) {
      throw ChimeraTK::logic_error("Cannot find device");
    }
  }

  void DMapFilesParser::getRegisterInfo(std::string dev_name, const std::string& reg_name, std::string& dev_file,
      uint32_t& reg_elem_nr, uint32_t& reg_offset, uint32_t& reg_size, uint32_t& reg_bar) {
    std::vector<std::pair<DeviceInfoMap::DeviceInfo, RegisterInfoMapPointer>>::iterator dmap_iter;
    RegisterInfoMap::RegisterInfo elem;

    if(dev_name == "" && _dmapElements.size() == 1) {
      dev_name = _dmapElements[0].first.deviceName;
    }
    dmap_iter = std::find_if(_dmapElements.begin(), _dmapElements.end(), findDevInPairByName_pred(dev_name));
    if(dmap_iter == _dmapElements.end()) {
      throw ChimeraTK::logic_error("Cannot find device " + dev_name);
    }
    (*dmap_iter).second->getRegisterInfo(reg_name, elem);
    reg_offset = elem.address;
    reg_size = elem.nBytes;
    reg_elem_nr = elem.nElements;
    reg_bar = elem.bar;
    dev_file = (*dmap_iter).first.uri;
  }

  void DMapFilesParser::getRegisterInfo(std::string dev_name,
      const std::string& reg_name,
      std::string& dev_file,
      RegisterInfoMap::RegisterInfo& elem) {
    std::vector<std::pair<DeviceInfoMap::DeviceInfo, RegisterInfoMapPointer>>::iterator dmap_iter;

    if(dev_name == "" && _dmapElements.size() == 1) {
      dev_name = _dmapElements[0].first.deviceName;
    }
    dmap_iter = std::find_if(_dmapElements.begin(), _dmapElements.end(), findDevInPairByName_pred(dev_name));
    if(dmap_iter == _dmapElements.end()) {
      throw ChimeraTK::logic_error("Cannot find device " + dev_name);
    }
    (*dmap_iter).second->getRegisterInfo(reg_name, elem);
    dev_file = (*dmap_iter).first.uri;
  }

  std::ostream& operator<<(std::ostream& os, const DMapFilesParser& dmfp) {
    std::vector<std::pair<DeviceInfoMap::DeviceInfo, RegisterInfoMapPointer>>::const_iterator iter;
    for(iter = dmfp._dmapElements.begin(); iter != dmfp._dmapElements.end(); ++iter) {
      os << (*iter).first << std::endl;
    }
    return os;
  }

  void DMapFilesParser::cleanAll() {
    _mapFiles.clear();
    _dmapElements.clear();
  }

  DMapFilesParser::~DMapFilesParser() { cleanAll(); }

  uint16_t DMapFilesParser::getdMapFileSize() { return _dmapElements.size(); }

  DMapFilesParser::DMapFilesParser() {}

  DMapFilesParser::DMapFilesParser(const std::string& dir) { parse_dir(dir); }

  DMapFilesParser::iterator DMapFilesParser::begin() { return _dmapElements.begin(); }

  DMapFilesParser::const_iterator DMapFilesParser::begin() const { return _dmapElements.begin(); }

  DMapFilesParser::iterator DMapFilesParser::end() { return _dmapElements.end(); }

  DMapFilesParser::const_iterator DMapFilesParser::end() const { return _dmapElements.end(); }
} // namespace ChimeraTK
