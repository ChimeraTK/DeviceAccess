/**
 *      @file           exlibmap.h
 *      @author         Adam Piotrowski <adam.piotrowski@desy.de>
 *      @version        1.0
 *      @brief          Provides exception classess for libmap                
 */
#ifndef MTCA4U_EXLIBMAP_H
#define	MTCA4U_EXLIBMAP_H
#include "exBase.h"
#include <string>

namespace mtca4u{

/**
 *      @brief  Provides base class for all exceptions from libmap                
 */
class exLibMap : public exBase {
public:

    enum {
        EX_CANNOT_OPEN_MAP_FILE, /**< Cannot open MAP file*/
        EX_CANNOT_OPEN_DMAP_FILE, /**< Cannot open DMAP file*/
        EX_FILES_CHECK_ERROR, /**< During correctness checking error or warning was detected*/
        EX_MAP_FILE_PARSE_ERROR, /**< During MAP file parsing error or warning was detected*/
        EX_NO_REGISTER_IN_MAP_FILE, /**< Specified register is not available in MAP file*/
        EX_NO_METADATA_IN_MAP_FILE, /**< Specified metadata is not available in MAP file*/
        EX_NO_DEVICE_IN_DMAP_FILE, /**< Specified device is not available in DMAP file*/
        EX_DMAP_FILE_PARSE_ERROR, /**< During DMAP file parsing error or warning was detected*/
        EX_NO_DATA_IN_DMAP_FILES /**< DMAP file is empty*/
    };
    /**
     * @brief Class constructor
     * 
     * @param _exMessage exception reason expressed as a string 
     * @param _exID exception reason expressed as a identifier 
     */
    exLibMap(const std::string &_exMessage, unsigned int _exID);

    virtual ~exLibMap() throw ();

    friend std::ostream& operator<<(std::ostream &os, const exLibMap& e);

};

/**
 *      @brief  Provides class for exceptions related to MAP file parsing                
 */
class exMapFile : public exLibMap {
public:
    /**
     * @brief Class constructor
     * 
     * @param _exMessage exception reason expressed as a string 
     * @param _exID exception reason expressed as a identifier 
     */
    exMapFile(const std::string &_exMessage, unsigned int _exID);
};

/**
 *      @brief  Provides class for exceptions related to MAP file                
 */
class exMapFileParser : public exMapFile {
public:
    /**
     * @brief Class constructor
     * 
     * @param _exMessage exception reason expressed as a string 
     * @param _exID exception reason expressed as a identifier 
     */
    exMapFileParser(const std::string &_exMessage, unsigned int _exID);
};

/**
 *      @brief  Provides class for exceptions related to DMAP file parsing                
 */
class exDmapFile : public exLibMap {
public:
    /**
     * @brief Class constructor
     * 
     * @param _exMessage exception reason expressed as a string 
     * @param _exID exception reason expressed as a identifier 
     */
    exDmapFile(const std::string &_exMessage, unsigned int _exID);
};

/**
 *      @brief  Provides class for exceptions related to DMAP                
 */
class exDmapFileParser : public exDmapFile {
public:
    /**
     * @brief Class constructor
     * 
     * @param _exMessage exception reason expressed as a string 
     * @param _exID exception reason expressed as a identifier 
     */
    exDmapFileParser(const std::string &_exMessage, unsigned int _exID);
};

}//namespace mtca4u

#endif	/* MTCA4U_EXLIBMAP_H */

