/**
 *      @file           mapFileParser.h
 *      @author         Adam Piotrowski <adam.piotrowski@desy.de>
 *      @version        1.0
 *      @brief          Provides method to parse DMAP file
 *                  
 */
#ifndef DMAPFILEPARSER_H
#define	DMAPFILEPARSER_H

#include "dmapFile.h"
#include <stdint.h>
#include <string>

/**
 * @brief  Provides method to parse DMAP file.
 * 
 * Class does not perform parsing of associated MAP files. 
 *      
 */
class dmapFileParser {
public:
    /**
     * @brief Performs parsing of specified DMAP file. Returns pointer to 
     * dmapElem object describing all devices in file. 
     *      
     * 
     * @throw exDmapFileParser [exLibMap::EX_DMAP_FILE_PARSE_ERROR] if parsing error 
     * detected, exMapFileParser [exLibMap::EX_CANNOT_OPEN_DMAP_FILE] if specified DMAP file cannot be opened
     * or exMapFileParser [exLibMap::EX_NO_DATA_IN_DMAP_FILES] if specified DMAP file is empty 
     * @param file_name name of DMAP file
     * @return pointer to dmapFile object   
     * 
     * @snippet test-libmap.cpp DMAP file parsing example    
     */
    ptrdmapFile parse(const std::string &file_name);
};

#endif	/* DMAPFILEPARSER_H */

