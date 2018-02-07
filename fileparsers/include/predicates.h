/**
 *      @file           predicates.h
 *      @author         Adam Piotrowski <adam.piotrowski@desy.de>
 *      @version        1.0
 *      @brief          Provides set of predicates for stl algorithms                
 */
#ifndef MTCA4U_PREDICATES_H
#define MTCA4U_PREDICATES_H

#include "DeviceInfoMap.h"
#include "RegisterInfoMap.h"

namespace ChimeraTK {

  /**
   *      @brief  Provides predicate to search registers by name
   */
  class findRegisterByName_pred {
    private:
      std::string _name, _module;
    public:

      findRegisterByName_pred(const std::string &name, const std::string &module)
    : _name(name), _module(module) {
      }

      bool operator()(const RegisterInfoMap::RegisterInfo& elem) {
        if ( (elem.name == _name) && (elem.module == _module) ){
          return true;
        }
        return false;
      }
  };

  /**
   *      @brief  Provides predicate to search metadata by name
   */
  class findMetaDataByName_pred {
    private:
      std::string _name;
    public:

      findMetaDataByName_pred(const std::string &name) : _name(name) {
      }

      bool operator()(const RegisterInfoMap::MetaData& elem) {
        if (elem.name == _name) return true;
        return false;
      }
  };

  /**
   *      @brief  Provides predicate to search device in pair by name
   */
  class findDevInPairByName_pred {
    private:
      std::string _name;
    public:

      findDevInPairByName_pred(const std::string &name) : _name(name) {
      }

      bool operator()(const std::pair<DeviceInfoMap::DeviceInfo, RegisterInfoMapPointer> & elem) {
        if (elem.first.deviceName == _name) return true;
        return false;
      }
  };

  /**
   *      @brief  Provides predicate to search device by name
   */
  class findDevByName_pred {
    private:
      std::string _name;
    public:

      findDevByName_pred(const std::string &name) : _name(name) {
      }

      bool operator()(const DeviceInfoMap::DeviceInfo& elem) {
        if (elem.deviceName == _name) return true;
        return false;
      }
  };

  /**
   *      @brief  Provides predicate to search map file by name
   */
  class findMapFileByName_pred {
    private:
      std::string _name;
    public:

      findMapFileByName_pred(const std::string &name) : _name(name) {
      }

      bool operator()(const RegisterInfoMapPointer map) {
        if (map->getMapFileName() == _name) return true;
        return false;
      }
  };

  /**
   *      @brief  Provides predicate to compare registers
   */
  class compareRegisterInfosByName_functor
  {
    public:
      bool operator()(const RegisterInfoMap::RegisterInfo& first, const RegisterInfoMap::RegisterInfo& second){
        if ( first.module == second.module ){
          return first.name < second.name;
        }
        else
          return first.module < second.module;
      }
  };

  /**
   *      @brief  Provides predicate to compare devices by name
   */
  class copmaredRegisterInfosByName_functor
  {
    public:
      bool operator()(const std::pair<DeviceInfoMap::DeviceInfo, RegisterInfoMapPointer> & first, const std::pair<DeviceInfoMap::DeviceInfo, RegisterInfoMapPointer> & second){
        return first.first.deviceName < second.first.deviceName;
      }
  };

  /**
   *      @brief  Provides predicate to compare devices by device file by name
   */
  class copmaredRegisterInfosByName2_functor
  {
    public:
      bool operator()(const DeviceInfoMap::DeviceInfo &first, const DeviceInfoMap::DeviceInfo &second){
        return first.deviceName < second.deviceName;
      }
  };

  /** Predicate to find RegisterInfoents in a std::vector by the module name. */
  class compareModuleName_pred{
    public:
      compareModuleName_pred(std::string const & moduleName) : _moduleName(moduleName){}
      bool operator()(const RegisterInfoMap::RegisterInfo & registerInfo) const{
        return (registerInfo.module == _moduleName);
      }
      typedef RegisterInfoMap::RegisterInfo argument_type;
    private:
      std::string _moduleName;
  };

}//namespace ChimeraTK

#endif  /* MTCA4U_PREDICATES_H */

