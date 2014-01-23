#include "dmapFilesParser.h"
#include "predicates.h"
#include "mapFile.h"
#include "exlibmap.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <dirent.h>
#include <sys/types.h>
#include <fstream>
#include <stdexcept>

namespace mtca4u{

void dmapFilesParser::parse_dirs(const std::vector<std::string> &dirs) {
    std::vector<std::string>::const_iterator iter;
    cleanAll();
    for (iter = dirs.begin(); iter != dirs.end(); iter++) {
        parse_one_directory(*iter);
    }
}

void dmapFilesParser::parse_dir(const std::string &dir) {
    cleanAll();
    parse_one_directory(dir);
}

void dmapFilesParser::parse_file(const std::string &fileName) {
    ptrdmapFile dmap;
    std::vector<dmapFile::dmapElem>::iterator dmap_elem_iter;
    std::vector<ptrmapFile>::iterator map_file_iter;
    ptrmapFile map;
    std::string dir = "";
    cleanAll();
    
    dmap = dmap_file_parser.parse(fileName);
    size_t pos = fileName.find_last_of('/');
    if (pos != std::string::npos){
        dir = fileName.substr(0, pos) + "/";
    }    
    for (dmap_elem_iter = dmap->dmap_file_elems.begin(); dmap_elem_iter != dmap->dmap_file_elems.end(); ++dmap_elem_iter) {
        map_file_iter = std::find_if(map_files.begin(), map_files.end(), findMapFileByName_pred((*dmap_elem_iter).map_file_name));
        if (map_file_iter == map_files.end()) {
            if ((*dmap_elem_iter).map_file_name[0] == '.') {
                map = map_file_parser.parse(dir + (*dmap_elem_iter).map_file_name);
            } else {
                map = map_file_parser.parse((*dmap_elem_iter).map_file_name);
            }
            map_files.push_back(map);
        } else {
            map = *map_file_iter;
        }
        dmap_elems.push_back(std::make_pair(*dmap_elem_iter, map));
    }
    if (dmap_elems.size() == 0) {
        throw exDmapFileParser("No data in dmap files", exLibMap::EX_NO_DATA_IN_DMAP_FILES);
    }
#ifdef __LIBMAP_WITH_ERROR_CHECKING__       
    dmapFile::errorList dmapErr;
    mapFile::errorList mapErr;
    std::ostringstream os;
    if (!check(dmapFile::errorList::errorElem::ERROR, mapFile::errorList::errorElem::ERROR, dmapErr, mapErr)) {
        os << dmapErr;
        os << mapErr;
        throw dmapFileParserEx(libmap_ex::EX_FILES_CHECK_ERROR, os.str());
    }
#endif //__LIBMAP_WITH_ERROR_CHECKING__   
}

void dmapFilesParser::parse_one_directory(const std::string &dir) {
    DIR *dp;
    struct dirent *dirp;
    size_t found;
    std::string file_name;
    ptrdmapFile dmap;
    std::vector<dmapFile::dmapElem>::iterator dmap_elem_iter;
    std::vector<ptrmapFile>::iterator map_file_iter;
    ptrmapFile map;
    std::string dir_new = dir;
    if (dir[dir.length() - 1] != '/')
        dir_new += "/";

    if ((dp = opendir(dir.c_str())) == NULL) {
        throw exDmapFileParser("Cannot open directory: \"" + dir + "\"", exLibMap::EX_CANNOT_OPEN_DMAP_FILE);
    }

    while ((dirp = readdir(dp)) != NULL) {
        if (dirp->d_type != DT_REG) {
            if (dirp->d_type != DT_UNKNOWN)
                continue;
        }
        file_name = std::string(dirp->d_name);
        found = file_name.find_last_of(".");
        if (found == std::string::npos)
            continue;
        if (file_name.substr(found) == ".dmap") {
            try {
                dmap = dmap_file_parser.parse(dir_new + file_name);
            } catch (exLibMap &e) {
                if (e.getID() == exLibMap::EX_NO_DATA_IN_DMAP_FILES) {
                    continue;
                }
                closedir(dp);
                throw;
            }
            for (dmap_elem_iter = dmap->dmap_file_elems.begin(); dmap_elem_iter != dmap->dmap_file_elems.end(); ++dmap_elem_iter) {
                map_file_iter = std::find_if(map_files.begin(), map_files.end(), findMapFileByName_pred((*dmap_elem_iter).map_file_name));
                if (map_file_iter == map_files.end()) {
                    try {
                        if ((*dmap_elem_iter).map_file_name[0] == '.'){ 
                            map = map_file_parser.parse(dir_new + (*dmap_elem_iter).map_file_name);
                        } else {
                            map = map_file_parser.parse((*dmap_elem_iter).map_file_name);
                        }
                    } catch (exLibMap &e) {
                        closedir(dp);
                        throw;
                    }
                    map_files.push_back(map);
                } else {
                    map = *map_file_iter;
                }
                dmap_elems.push_back(std::make_pair(*dmap_elem_iter, map));
            }
        }
    }
    closedir(dp);
    if (dmap_elems.size() == 0) {
        throw exDmapFileParser("No data in dmap files", exLibMap::EX_NO_DATA_IN_DMAP_FILES);
    }

#ifdef __LIBMAP_WITH_ERROR_CHECKING__       
    dmapFile::errorList dmapErr;
    mapFile::errorList mapErr;
    std::ostringstream os;
    if (!check(dmapFile::errorList::errorElem::ERROR, mapFile::errorList::errorElem::ERROR, dmapErr, mapErr)) {
        os << dmapErr;
        os << mapErr;
        throw dmapFileParserEx(libmap_ex::EX_FILES_CHECK_ERROR, os.str());
    }
#endif //__LIBMAP_WITH_ERROR_CHECKING__   
}

//FIXME: Why is dlevel not used?
bool dmapFilesParser::check(dmapFile::errorList::errorElem::TYPE /*dlevel*/, mapFile::errorList::errorElem::TYPE mlevel, dmapFile::errorList &dmap_err, mapFile::errorList &map_err) {

    std::vector<std::pair<dmapFile::dmapElem, ptrmapFile> > dmaps = dmap_elems;
    std::vector<std::pair<dmapFile::dmapElem, ptrmapFile> >::iterator iter_p, iter_n;
    bool ret = true;

    dmap_err.clear();
    map_err.clear();
    if (dmaps.size() < 2) {
        return true;
    }

    std::sort(dmaps.begin(), dmaps.end(), copmaredMapElemsByName_functor());
    iter_p = dmaps.begin();
    iter_n = iter_p + 1;
    while (1) {
        if ((*iter_p).first.dev_name == (*iter_n).first.dev_name) {
            if ((*iter_p).first.dev_file != (*iter_n).first.dev_file || (*iter_p).first.map_file_name != (*iter_n).first.map_file_name) {
                dmap_err.insert(dmapFile::errorList::errorElem(dmapFile::errorList::errorElem::ERROR, dmapFile::errorList::errorElem::NONUNIQUE_DEVICE_NAME, (*iter_p).first, (*iter_n).first));
                ret = false;
            }
        }
        iter_p = iter_n;
        iter_n = ++iter_n;
        if (iter_n == dmaps.end()) {
            break;
        }
    }

    mapFile::errorList mapErr;
    std::vector<ptrmapFile>::iterator map_iter;
    for (map_iter = map_files.begin(); map_iter != map_files.end(); map_iter++) {
        if (!(*map_iter)->check(mapErr, mlevel)) {
            map_err.errors.splice(map_err.errors.end(), mapErr.errors);
            ret = false;
        }
    }

    return ret;
}

ptrmapFile dmapFilesParser::getMapFile(const std::string &dev_name) {
    std::vector<std::pair<dmapFile::dmapElem, ptrmapFile> >::iterator dmap_iter;
    dmap_iter = std::find_if(dmap_elems.begin(), dmap_elems.end(), findDevInPairByName_pred(dev_name));
    if (dmap_iter == dmap_elems.end()) {
        throw exDmapFileParser("Cannot find device " + dev_name, exLibMap::EX_NO_DEVICE_IN_DMAP_FILE);
    }
    return (*dmap_iter).second;
}

void dmapFilesParser::getdMapFileElem(const std::string &dev_name, dmapFile::dmapElem &dMapFileElem) {
    dMapFileElem = getdMapFileElem( dev_name );
}

dmapFile::dmapElem const & dmapFilesParser::getdMapFileElem(const std::string &dev_name) {
    std::vector<std::pair<dmapFile::dmapElem, ptrmapFile> >::iterator dmap_iter;
    dmap_iter = std::find_if(dmap_elems.begin(), dmap_elems.end(), findDevInPairByName_pred(dev_name));
    if (dmap_iter == dmap_elems.end()) {
        throw exDmapFileParser("Cannot find device " + dev_name, exLibMap::EX_NO_DEVICE_IN_DMAP_FILE);
    }
    return (*dmap_iter).first;
}

void dmapFilesParser::getdMapFileElem(int elem_nr, dmapFile::dmapElem &dMapFileElem) {
    try {
        dMapFileElem = dmap_elems.at(elem_nr).first;
    } catch (std::out_of_range) {
        throw exDmapFileParser("Cannot find device", exLibMap::EX_NO_DEVICE_IN_DMAP_FILE);
    }
}

void dmapFilesParser::getRegisterInfo(std::string dev_name, const std::string &reg_name, std::string& dev_file, uint32_t& reg_elem_nr, uint32_t& reg_offset, uint32_t& reg_size, uint32_t& reg_bar) {
    std::vector<std::pair<dmapFile::dmapElem, ptrmapFile> >::iterator dmap_iter;
    mapFile::mapElem elem;

    if (dev_name == "" && dmap_elems.size() == 1) {
        dev_name = dmap_elems[0].first.dev_name;
    }
    dmap_iter = std::find_if(dmap_elems.begin(), dmap_elems.end(), findDevInPairByName_pred(dev_name));
    if (dmap_iter == dmap_elems.end()) {
        throw exDmapFileParser("Cannot find device " + dev_name, exLibMap::EX_NO_DEVICE_IN_DMAP_FILE);
    }
    (*dmap_iter).second->getRegisterInfo(reg_name, elem);
    reg_offset = elem.reg_address;
    reg_size = elem.reg_size;
    reg_elem_nr = elem.reg_elem_nr;
    reg_bar = elem.reg_bar;
    dev_file = (*dmap_iter).first.dev_file;
}

void dmapFilesParser::getRegisterInfo(std::string dev_name, const std::string &reg_name, std::string& dev_file, mapFile::mapElem &elem) {
    std::vector<std::pair<dmapFile::dmapElem, ptrmapFile> >::iterator dmap_iter;

    if (dev_name == "" && dmap_elems.size() == 1) {
        dev_name = dmap_elems[0].first.dev_name;
    }
    dmap_iter = std::find_if(dmap_elems.begin(), dmap_elems.end(), findDevInPairByName_pred(dev_name));
    if (dmap_iter == dmap_elems.end()) {
        throw exDmapFileParser("Cannot find device " + dev_name, exLibMap::EX_NO_DEVICE_IN_DMAP_FILE);
    }
    (*dmap_iter).second->getRegisterInfo(reg_name, elem);
    dev_file = (*dmap_iter).first.dev_file;
}

std::ostream& operator<<(std::ostream &os, const dmapFilesParser& dmfp) {
    std::vector<std::pair<dmapFile::dmapElem, ptrmapFile> >::const_iterator iter;
    for (iter = dmfp.dmap_elems.begin(); iter != dmfp.dmap_elems.end(); ++iter) {
        std::cout << (*iter).first << std::endl;
    }
    return os;
}

void dmapFilesParser::cleanAll() {
    map_files.clear();
    dmap_elems.clear();
}

dmapFilesParser::~dmapFilesParser() {
    cleanAll();
}

uint16_t dmapFilesParser::getdMapFileSize() {
    return dmap_elems.size();
}

dmapFilesParser::dmapFilesParser()
{
}

dmapFilesParser::dmapFilesParser(const std::string &dir) {
    parse_dir(dir);
}


dmapFilesParser::iterator dmapFilesParser::begin() {
    return dmap_elems.begin();
}

dmapFilesParser::const_iterator dmapFilesParser::begin() const{
    return dmap_elems.begin();
}

dmapFilesParser::iterator dmapFilesParser::end() {
    return dmap_elems.end();
}

dmapFilesParser::const_iterator dmapFilesParser::end() const{
    return dmap_elems.end();
}

}//namespace mtca4u
