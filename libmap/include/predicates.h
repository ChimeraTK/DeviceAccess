/**
 *      @file           predicates.h
 *      @author         Adam Piotrowski <adam.piotrowski@desy.de>
 *      @version        1.0
 *      @brief          Provides set of predicates for stl algorithms                
 */
#ifndef PREDICATES_H
#define	PREDICATES_H

#include "mapFile.h"
#include "dmapFile.h"

/**
 *      @brief  Provides predicate to search registers by name
 */
class findRegisterByName_pred {
private:
    std::string name;
public:

    findRegisterByName_pred(const std::string &_name) : name(_name) {
    }

    bool operator()(const mapFile::mapElem& elem) {
        if (elem.reg_name == name) return true;
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
        return first.reg_name < second.reg_name;
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
        return first.dev_name < first.dev_name;
    }
};

#endif	/* PREDICATES_H */

