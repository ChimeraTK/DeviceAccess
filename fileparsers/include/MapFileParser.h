/**
 *      @file           MapFileParser.h
 *      @author         Adam Piotrowski <adam.piotrowski@desy.de>
 *      @brief          Provides method to parse MAP file
 *                  
 */

#ifndef MTCA4U_MAPFILEPARSER_H
#define	MTCA4U_MAPFILEPARSER_H


#include "RegisterInfoMap.h"
#include <iomanip>
#include <stdint.h>
#include <string>
#include <fstream>


namespace mtca4u{

/**
 * @brief  Provides method to parse MAP file 
 *      
 */
class mapFileParser {
public:
    /**
     * @brief Performs parsing of specified MAP file. Returns pointer to 
     * RegisterInfo object describing all registers and metadata available in file. 
     *      
     * 
     * @throw exMapFileParser [exLibMap::EX_MAP_FILE_PARSE_ERROR] if parsing error 
     * detected or exMapFileParser [exLibMap::EX_CANNOT_OPEN_MAP_FILE] if specified MAP file cannot be opened
     * @param file_name name of MAP file
     * @return pointer to RegisterInfoMap object    
     * 
     * @snippet test-libmap.cpp MAP file parsing example 
     *     
     */
    ptrmapFile    parse(const std::string &file_name);

    /** Split the string at the last dot. The part up to the last dot is the first returned argument,
     *	the part after the last dot is the second. Hence, the first part can contain dots itself, the second
     *	part cannot. If there is no dot, the first part is empty and the full string is returned as second 
     *  (the part up to the first dot is considered as prefix).
     */
    static std::pair<std::string, std::string> splitStringAtLastDot( std::string moduleDotName);
};

}//namespace mtca4u

#endif	/* MTCA4U_MAPFILEPARSER_H */

