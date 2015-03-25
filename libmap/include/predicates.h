/**
 *      @file           predicates.h
 *      @author         Adam Piotrowski <adam.piotrowski@desy.de>
 *      @version        1.0
 *      @brief          Provides set of predicates for stl algorithms                
 */
#ifndef MTCA4U_PREDICATES_H
#define	MTCA4U_PREDICATES_H

#include "mapFile.h"
#include "dmapFile.h"

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

    bool operator()(const mapFile::mapElem& elem) {
      if ( (elem.reg_name == name) && (elem.reg_module == module) ){
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
    std::string name;
public:

    findMetaDataByName_pred(const std::string &_name) : name(_name) {
    }

    bool operator()(const mapFile::metaData& elem) {
        if (elem.name == name) return true;
        return false;
    }
};

/**
 *      @brief  Provides predicate to search device in pair by name
 */
class findDevInPairByName_pred {
private:
    std::string name;
public:

    findDevInPairByName_pred(const std::string &_name) : name(_name) {
    }

    bool operator()(const std::pair<dmapFile::dmapElem, ptrmapFile> & elem) {
      if (elem.first.dev_name == name) return true;
        return false;
    }
};

/**
 *      @brief  Provides predicate to search device by name
 */
class findDevByName_pred {
private:
    std::string name;
public:

    findDevByName_pred(const std::string &_name) : name(_name) {
    }

    bool operator()(const dmapFile::dmapElem& elem) {
        if (elem.dev_name == name) return true;
        return false;
    }
};

/**
 *      @brief  Provides predicate to search map file by name
 */
class findMapFileByName_pred {
private:
    std::string name;
public:

    findMapFileByName_pred(const std::string &_name) : name(_name) {
    }

    bool operator()(const ptrmapFile map) {
        if (map->getMapFileName() == name) return true;
        return false;
    }
};

/**
 *      @brief  Provides predicate to compare registers
 */
class compareMapElemsByName_functor
{
public:
    bool operator()(const mapFile::mapElem& first, const mapFile::mapElem& second){
        if ( first.reg_module == second.reg_module ){
	    return first.reg_name < second.reg_name;
	}
	else
	    return first.reg_module < second.reg_module;	    
    }
};

/**
 *      @brief  Provides predicate to compare devices by name
 */
class copmaredMapElemsByName_functor
{
public:
    bool operator()(const std::pair<dmapFile::dmapElem, ptrmapFile> & first, const std::pair<dmapFile::dmapElem, ptrmapFile> & second){
        return first.first.dev_name < second.first.dev_name;
    }
};

/**
 *      @brief  Provides predicate to compare devices by device file by name
 */
class copmaredMapElemsByName2_functor
{
public:
    bool operator()(const dmapFile::dmapElem &first, const dmapFile::dmapElem &second){
        return first.dev_name < second.dev_name;
    }
};

/** Predicate to find mapElements in a std::vector by the module name. */
class compareModuleName_pred{
 public:
 compareModuleName_pred(std::string const & moduleName) : _moduleName(moduleName){}
  bool operator()(const mapFile::mapElem & me) const{
    return (me.reg_module == _moduleName);
  }
  typedef mapFile::mapElem argument_type;
 private:
  std::string _moduleName;
};

}//namespace mtca4u

#endif	/* MTCA4U_PREDICATES_H */

