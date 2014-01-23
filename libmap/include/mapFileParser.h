/**
 *      @file           mapFileParser.h
 *      @author         Adam Piotrowski <adam.piotrowski@desy.de>
 *      @version        1.0
 *      @brief          Provides method to parse MAP file
 *                  
 */

#ifndef MTCA4U_MAPFILEPARSER_H
#define	MTCA4U_MAPFILEPARSER_H


#include "mapFile.h"
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
     * mapElem object describing all registers and matadata available in file. 
     *      
     * 
     * @throw exMapFileParser [exLibMap::EX_MAP_FILE_PARSE_ERROR] if parsing error 
     * detected or exMapFileParser [exLibMap::EX_CANNOT_OPEN_MAP_FILE] if specified MAP file cannot be opened
     * @param file_name name of MAP file
     * @return pointer to mapFile object    
     * 
     * @snippet test-libmap.cpp MAP file parsing example 
     *     
     */
    ptrmapFile    parse(const std::string &file_name);
};

}//namespace mtca4u

#endif	/* MTCA4U_MAPFILEPARSER_H */

