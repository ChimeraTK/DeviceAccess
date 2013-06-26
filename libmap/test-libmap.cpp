/**
 *      @file           test-libmap.cpp
 *      @author         Adam Piotrowski <adam.piotrowski@desy.de>
 *      @version        1.0
 *      @brief          Testbench for libmap class               
 */
#include <cstdlib>
#include <iostream>
#include <sstream>
#include "./include/libmap.h"

using namespace std;

void printInfo(const std::string& msg) {
    std::ostringstream os;
    os << "=" << std::setw(msg.length() + (91 - msg.length()) / 2) << std::right << msg << std::setw((91 - msg.length()) / 2 + (msg.length() % 2 ? 0 : 1)) << std::right << "=";
    std::cout << "============================================================================================" << std::endl;
    std::cout << os.str().c_str() << std::endl;
    std::cout << "============================================================================================" << std::endl;
}

int main(int /*argc*/, char** /*argv*/) {
    /*=============================================================================================================*/
    printInfo("MAP: File parsing");
    /** [MAP file parsing example] */
    mapFileParser mapFileParser;
    ptrmapFile ptrMap;
    try {
        ptrMap = mapFileParser.parse("./sis_demo.map");
        std::cout << ptrMap << std::endl;
    } catch (exLibMap &e) {
        std::cout << e << " (" << e.what() << ")" << std::endl;
        return -1;
    }
    /** [MAP file parsing example] */
    /*=============================================================================================================*/
    printInfo("MAP: Iterating throught all MAP file registers");
    /** [MAP iterating throught all registers] */
    /* ptrMap is a pointer to mapFile object filled by function mapFileParser::parse */
    mapFile::iterator mapIter;
    for (mapIter = ptrMap->begin(); mapIter != ptrMap->end(); ++mapIter) {
        std::cout << *mapIter << std::endl;
    }
    /** [MAP iterating throught all registers] */
    /*=============================================================================================================*/
    printInfo("MAP: File correctness checking");
    /** [MAP file correctness checking] */
    mapFile::errorList err;
    /* ptrMap is a pointer to mapFile object filled by function mapFileParser::parse */
    if (!ptrMap->check(err, mapFile::errorList::errorElem::WARNING)) {
        std::cout << err << std::endl;
    }
    /** [MAP file correctness checking] */
    /*=============================================================================================================*/
    printInfo("MAP: Getting register info and metadata");
    /** [MAP getting info and metadata]*/
    /* ptrMap is a pointer to mapFile object filled by function mapFileParser::parse */
    std::string metaData;
    mapFile::mapElem mapElem;
    try {
        ptrMap->getMetaData("HW_VERSION", metaData);
        ptrMap->getRegisterInfo("AREA_DAQ0", mapElem);
    } catch (exLibMap &e) {
        std::cout << e << " (" << e.what() << ")" << std::endl;
        return -1;
    }
    std::cout << "METADATA \"HW_VERSION\": " << metaData << std::endl;
    std::cout << "MAP DATA \"AREA_DAQ0\" : " << mapElem << std::endl;
    /** [MAP getting info and metadata]*/
    /*=============================================================================================================*/
    printInfo("DMAP: File parsing");
    /** [DMAP file parsing example] */
    ptrdmapFile ptrDmap;
    dmapFileParser dmapParser;
    try {
        ptrDmap = dmapParser.parse("dev_map_file.dmap");
        std::cout << ptrDmap << std::endl;
    } catch (exLibMap &e) {
        std::cout << e << " (" << e.what() << ")" << std::endl;
        return -1;
    }
    /** [DMAP file parsing example] */
    /*=============================================================================================================*/
    printInfo("DMAP: Iterating throught all devices");
    /** [DMAP iterating throught all devices] */
    dmapFile::iterator dmapIter;
    /* ptrdmap is a pointer to dmapFile object filled by function dmapFileParser::parse */
    for (dmapIter = ptrDmap->begin(); dmapIter != ptrDmap->end(); ++dmapIter) {
        std::cout << *dmapIter << std::endl;
    }
    /** [DMAP iterating throught all devices] */
    /*=============================================================================================================*/
    printInfo("DMAP: File correctness checking");
    /** [DMAP file correctness checking] */
    dmapFile::errorList derr;
    /* ptrDmap is a pointer to dmapFile object filled by function dmapFileParser::parse */
    if (!ptrDmap->check(derr, dmapFile::errorList::errorElem::WARNING)) {
        std::cout << derr << std::endl;
    }
    /** [DMAP file correctness checking] */
    /*=============================================================================================================*/
    printInfo("DMAP: Getting device info");
    /** [DMAP getting info]*/
    /* ptrDmap is a pointer to dmapFile object filled by function dmapFileParser::parse */
    dmapFile::dmapElem dmapElem;
    try {
        ptrDmap->getDeviceInfo("ADC", dmapElem);
    } catch (exLibMap &e) {
        std::cout << e << " (" << e.what() << ")" << std::endl;
        return -1;
    }
    std::cout << "DEV \"ADC\": " << dmapElem << std::endl;
    /** [DMAP getting info]*/    
    /*=============================================================================================================*/
    printInfo("DMAPS: dir parsing");
    /** [DMAPS dir parsing]*/
    dmapFilesParser dmap_FilesParser;
    try {
        dmap_FilesParser.parse_dir("./");
        std::cout << dmap_FilesParser << std::endl;
    } catch (exLibMap &e) {
        std::cout << e << " (" << e.what() << ")" << std::endl;
        return -1;
    }
    /** [DMAPS dir parsing]*/
    /*=============================================================================================================*/
    printInfo("DMAPS: dirs parsing");
    /** [DMAPS dirs parsing]*/
    try {
        std::vector<std::string> dirs;
        dirs.push_back("./");
        dirs.push_back("./dmaps_ok");
        dmap_FilesParser.parse_dirs(dirs);
        std::cout << dmap_FilesParser << std::endl;
    } catch (exLibMap &e) {
        std::cout << e << " (" << e.what() << ")" << std::endl;
        return -1;
    }
    /** [DMAPS dirs parsing]*/
    /*=============================================================================================================*/
    printInfo("DMAPS: file parsing");
    /** [DMAPS file parsing]*/
    try {
        dmap_FilesParser.parse_file("./dmaps_ok/dev_map_file.dmap");
        std::cout << dmap_FilesParser << std::endl;
    } catch (exLibMap &e) {
        std::cout << e << " (" << e.what() << ")" << std::endl;
        return -1;
    }
    /** [DMAPS file parsing]*/
    /*=============================================================================================================*/
    printInfo("DMAPS: checking");
    /** [DMAPS checking]*/
    dmapFile::errorList d_err;
    mapFile::errorList m_err;
    if (!dmap_FilesParser.check(dmapFile::errorList::errorElem::ERROR, mapFile::errorList::errorElem::ERROR, d_err, m_err)) {
        std::cout << d_err << std::endl;
        std::cout << m_err << std::endl;
    }
    /** [DMAPS checking]*/
    /*=============================================================================================================*/
    printInfo("DMAPS: getting dev end reg info");
    /** [DMAPS get reg info]*/
    try {        
        std::string devFile;
        mapFile::mapElem regElem;
        dmap_FilesParser.getRegisterInfo("ADC", "WORD_FIRMWARE", devFile, regElem);
        std::cout << "DEV: " << devFile << std::endl << "ADC:WORD_FIRMWARE ->" << regElem << std::endl;;  
    } catch (exLibMap &e) {
        std::cout << e << " (" << e.what() << ")" << std::endl;
        return -1;
    }
    /** [DMAPS get reg info]*/
    /*=============================================================================================================*/
    printInfo("DMAPS: iterating");
    /** [DMAPS iterate]*/
    dmapFilesParser::iterator dmapFilesIter;    
    for (dmapFilesIter = dmap_FilesParser.begin(); dmapFilesIter != dmap_FilesParser.end(); ++dmapFilesIter){
        std::cout << (*dmapFilesIter).first << std::endl;
        std::cout << (*dmapFilesIter).second << std::endl;
    }
    /** [DMAPS iterate]*/
    
    dmap_FilesParser.getdMapFileElem("ADC", dmapElem);
    std::cout << dmapElem << std::endl;
    
    
    
    return 0;
#if 0    
    mapFileParser map_FileParser;
    dmapFilesParser dmap_FilesParser;
    dmapFilesParser::iterator iter;
    ptrmapFile ptr_map;
    mapFile::iterator map_iter;

    try {
        dmap_FilesParser.parse_one_file("dev_map_file.dmap");
    } catch (exLibMap &e) {
        std::cout << e << "(" << e.what() << ")" << std::endl;
    }

    for (iter = dmap_FilesParser.begin(); iter != dmap_FilesParser.end(); ++iter) {
        std::cout << *iter << std::endl;
    }

    try {
        ptr_map = map_FileParser.parse("./sis_demo.map");
    } catch (exLibMap &e) {
        std::cout << e << std::endl;
    }

    for (map_iter = ptr_map->begin(); map_iter != ptr_map->end(); ++map_iter) {
        std::cout << *map_iter << std::endl;
    }

    return 0;

    //mapFileParser map_FileParser;
    dmapFileParser dmap_FileParser;
    //dmapFilesParser dmap_FilesParser;
    //ptrmapFile ptr_map;
    ptrdmapFile ptr_dmap;

    uint32_t reg_elem_nr;
    uint32_t reg_offset;
    uint32_t reg_size;
    uint32_t reg_bar;
    std::string dev_file;

    try {
        dmap_FilesParser.parse_one_file("dev_map_file.dmap");
    } catch (exLibMap &e) {
        std::cout << e << "(" << e.what() << ")" << std::endl;

    }
    //  return 0;
    /*=============================================================================*/
    std::cout << "Parsing ./sis_demo.map" << std::endl;
    std::string metaData;
    mapFile::errorList err;
    mapFile::mapElem mapElem;
    try {
        ptr_map = map_FileParser.parse("./sis_demo.map");
        ptr_map->getMetaData("HW_VERSION", metaData);
        ptr_map->getRegisterInfo("AREA_DAQ1", mapElem);
    } catch (exLibMap &e) {
        std::cout << e << std::endl;
    }
    std::cout << ptr_map << std::endl;
    std::cout << "METADATA: " << metaData << std::endl;
    std::cout << "MAP DATA: " << mapElem << std::endl;
    if (!ptr_map->check(err, mapFile::errorList::errorElem::ERROR)) {
        std::cout << err << std::endl;
    }

    /*=============================================================================*/
    std::cout << "Parsing ./noexist_sis_demo.map: ";
    try {
        ptr_map = map_FileParser.parse("./noexist_sis_demo.map");
    } catch (exLibMap &e) {
        std::cout << e << std::endl;
    }
    /*=============================================================================*/
    std::cout << "Parsing ./sis_error_demo.map: ";
    try {
        ptr_map = map_FileParser.parse("./sis_error_demo.map");
    } catch (exLibMap &e) {
        std::cout << e << std::endl;
    }
    /*=============================================================================*/
    std::cout << "Parsing ./dev_map_file.dmap" << std::endl;
    dmapFile::dmapElem dmapElem;
    dmapFile::errorList derr;
    try {
        ptr_dmap = dmap_FileParser.parse("./dev_map_file.dmap");
        ptr_dmap->getDeviceInfo("ADC", dmapElem);
    } catch (exLibMap &e) {
        std::cout << e << std::endl;
    }
    std::cout << ptr_dmap << std::endl;
    std::cout << dmapElem << std::endl;
    if (!ptr_dmap->check(derr, dmapFile::errorList::errorElem::ERROR)) {
        std::cout << derr << std::endl;
    }

    /*=============================================================================*/
    std::cout << "Parsing ./dev_nonexist_map_file.dmap: ";
    try {
        ptr_dmap = dmap_FileParser.parse("./dev_nonexist_map_file.dmap");
    } catch (exLibMap &e) {
        std::cout << e << std::endl;
    }
    /*=============================================================================*/
    std::cout << "Parsing ./dev_empty_map_file.dmap: ";
    try {
        ptr_dmap = dmap_FileParser.parse("./dev_empty_map_file.dmap");
    } catch (exLibMap &e) {
        std::cout << e << std::endl;
    }
    /*=============================================================================*/
    std::cout << "Parsing ./dev_error_map_file.dmap: ";
    try {
        ptr_dmap = dmap_FileParser.parse("./dev_error_map_file.dmap");
    } catch (exLibMap &e) {
        std::cout << e << std::endl;
    }
    /*=============================================================================*/
    std::cout << "Parsing  all dmap in directory " << std::endl;
    mapFile::mapElem elem;
    try {
        dmap_FilesParser.parse("./dmaps_ok");
        dmap_FilesParser.getRegisterInfo("ADC", "WORD_COMPILATION", dev_file, reg_elem_nr, reg_offset, reg_size, reg_bar);
        dmap_FilesParser.getRegisterInfo("ADC", "WORD_COMPILATION", dev_file, elem);
    } catch (exLibMap &e) {
        std::cout << e << std::endl;
    }

    if (!dmap_FilesParser.check(dmapFile::errorList::errorElem::ERROR, mapFile::errorList::errorElem::ERROR, derr, err)) {
        std::cout << derr << std::endl;
        std::cout << err << std::endl;
    }

    std::cout << dmap_FilesParser << std::endl;
    std::cout << "DEV     : ADC" << std::endl;
    std::cout << "REG     : WORD_COMPILATION" << std::endl;
    std::cout << "FILE    : " << dev_file << std::endl;
    std::cout << "REG NR  : " << reg_elem_nr << std::endl;
    std::cout << "OFFSET  : " << reg_offset << std::endl;
    std::cout << "REG SIZE: " << reg_size << std::endl;
    std::cout << "REG BAR : " << reg_bar << std::endl;
    std::cout << elem << std::endl;

#endif
    return 0;
}

