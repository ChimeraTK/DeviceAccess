/**
 *      @file           predicates.h
 *      @author         Adam Piotrowski <adam.piotrowski@desy.de>
 *      @version        1.0
 *      @brief          Provides set of predicates for stl algorithms                
 */
#ifndef MTCA4U_PREDICATES_H
#define	MTCA4U_PREDICATES_H

#include "DeviceInfoMap.h"
#include "RegisterInfoMap.h"

namespace mtca4u{

/**
 *      @brief  Provides predicate to search registers by name
 */
class findRegisterByName_pred {
private:
	std::string name, module;
public:

	findRegisterByName_pred(const std::string &_name, const std::string &_module)
	: name(_name), module(_module) {
	}

	bool operator()(const RegisterInfoMap::RegisterInfo& elem) {
		if ( (elem._name == name) && (elem._module == module) ){
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
		if (elem._name == _name) return true;
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
		if (elem.first._deviceName == _name) return true;
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
		if (elem._deviceName == _name) return true;
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
		if ( first._module == second._module ){
			return first._name < second._name;
		}
		else
			return first._module < second._module;
	}
};

/**
 *      @brief  Provides predicate to compare devices by name
 */
class copmaredRegisterInfosByName_functor
{
public:
	bool operator()(const std::pair<DeviceInfoMap::DeviceInfo, RegisterInfoMapPointer> & first, const std::pair<DeviceInfoMap::DeviceInfo, RegisterInfoMapPointer> & second){
		return first.first._deviceName < second.first._deviceName;
	}
};

/**
 *      @brief  Provides predicate to compare devices by device file by name
 */
class copmaredRegisterInfosByName2_functor
{
public:
	bool operator()(const DeviceInfoMap::DeviceInfo &first, const DeviceInfoMap::DeviceInfo &second){
		return first._deviceName < second._deviceName;
	}
};

/** Predicate to find RegisterInfoents in a std::vector by the module name. */
class compareModuleName_pred{
public:
	compareModuleName_pred(std::string const & moduleName) : _moduleName(moduleName){}
	bool operator()(const RegisterInfoMap::RegisterInfo & registerInfo) const{
		return (registerInfo._module == _moduleName);
	}
	typedef RegisterInfoMap::RegisterInfo argument_type;
private:
	std::string _moduleName;
};

}//namespace mtca4u

#endif	/* MTCA4U_PREDICATES_H */

