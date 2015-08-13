#include "DMapFilesParser.h"
#include "predicates.h"
#include "MapFile.h"
#include "MapException.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <dirent.h>
#include <sys/types.h>
#include <fstream>
#include <stdexcept>
//#include <boost/filesystem.hpp>

namespace mtca4u{

  void DMapFilesParser::parse_dirs(const std::vector<std::string> &dirs) {
    std::vector<std::string>::const_iterator iter;
    cleanAll();
    for (iter = dirs.begin(); iter != dirs.end(); iter++) {
      parse_one_directory(*iter);
    }
  }

  void DMapFilesParser::parse_dir(const std::string &dir) {
    cleanAll();
    parse_one_directory(dir);
  }

  void DMapFilesParser::parse_file(const std::string &fileName) {
    ptrdmapFile dmap;
    std::vector<DMapFile::dmapElem>::iterator dmap_elem_iter;
    std::vector<ptrmapFile>::iterator map_file_iter;
    ptrmapFile map;
    std::string absolutePathToDMapDir = getCurrentWorkingDirectory();
    size_t pos = fileName.find_last_of('/');
    if (pos != std::string::npos) {
      combinePaths(absolutePathToDMapDir, fileName.substr(0, pos));
    }
    cleanAll();
    dmap = dmap_file_parser.parse(fileName);
    for (dmap_elem_iter = dmap->dmap_file_elems.begin();
         dmap_elem_iter != dmap->dmap_file_elems.end(); ++dmap_elem_iter) {
      std::string absPathToCurrentMapFile = absolutePathToDMapDir;
      combinePaths(absPathToCurrentMapFile, dmap_elem_iter->map_file_name);
      map_file_iter = std::find_if(map_files.begin(), map_files.end(),
                                   findMapFileByName_pred(absPathToCurrentMapFile));
      if (map_file_iter == map_files.end()) {
        map = map_file_parser.parse(absPathToCurrentMapFile);
        map_files.push_back(map);
      } else {
        map = *map_file_iter;
      }
      dmap_elems.push_back(std::make_pair(*dmap_elem_iter, map));
    }
  #ifdef __LIBMAP_WITH_ERROR_CHECKING__
    DMapFile::errorList dmapErr;
    mapFile::errorList mapErr;
    std::ostringstream os;
    if (!check(DMapFile::errorList::errorElem::ERROR,
               mapFile::errorList::errorElem::ERROR, dmapErr, mapErr)) {
      os << dmapErr;
      os << mapErr;
      throw dmapFileParserEx(libmap_ex::EX_FILES_CHECK_ERROR, os.str());
    }
  #endif  //__LIBMAP_WITH_ERROR_CHECKING__
  }

void DMapFilesParser::parse_one_directory(const std::string &dir) {
  DIR *dp;
  struct dirent *dirp;
  size_t found;
  std::string file_name;
  ptrdmapFile dmap;
  std::vector<DMapFile::dmapElem>::iterator dmap_elem_iter;
  std::vector<ptrmapFile>::iterator map_file_iter;
  ptrmapFile map;
  std::string dir_new = dir;
  if (dir[dir.length() - 1] != '/')
    dir_new += "/";

  if ((dp = opendir(dir.c_str())) == NULL) {
    throw DMapFileParserException("Cannot open directory: \"" + dir + "\"", LibMapException::EX_CANNOT_OPEN_DMAP_FILE);
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
      } catch (LibMapException &e) {
	if (e.getID() == LibMapException::EX_NO_DMAP_DATA) {
	  continue;
	}
	closedir(dp);
	throw;
      }
      /* fixme? repetition from parse_file structure this better?*/
	  for (dmap_elem_iter = dmap->dmap_file_elems.begin(); dmap_elem_iter != dmap->dmap_file_elems.end(); ++dmap_elem_iter) {
	    map_file_iter = std::find_if(map_files.begin(), map_files.end(), findMapFileByName_pred((*dmap_elem_iter).map_file_name));
	    if (map_file_iter == map_files.end()) {
	      try {
		if ((*dmap_elem_iter).map_file_name[0] == '.'){
		  map = map_file_parser.parse(dir_new + (*dmap_elem_iter).map_file_name);
		} else {
		  map = map_file_parser.parse((*dmap_elem_iter).map_file_name);
		}
	      } catch (LibMapException &e) {
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
    //TODO: change message? No dmap files in dir
    throw DMapFileParserException("DMAP file is empty or does not exist", LibMapException::EX_NO_DMAP_DATA);
  }

#ifdef __LIBMAP_WITH_ERROR_CHECKING__       
  DMapFile::errorList dmapErr;
  mapFile::errorList mapErr;
  std::ostringstream os;
  if (!check(DMapFile::errorList::errorElem::ERROR, mapFile::errorList::errorElem::ERROR, dmapErr, mapErr)) {
    os << dmapErr;
    os << mapErr;
    throw dmapFileParserEx(libmap_ex::EX_FILES_CHECK_ERROR, os.str());
  }
#endif //__LIBMAP_WITH_ERROR_CHECKING__   
}

//FIXME: Why is dlevel not used?
bool DMapFilesParser::check(DMapFile::errorList::errorElem::TYPE /*dlevel*/, mapFile::errorList::errorElem::TYPE mlevel, DMapFile::errorList &dmap_err, mapFile::errorList &map_err) {

  std::vector<std::pair<DMapFile::dmapElem, ptrmapFile> > dmaps = dmap_elems;
  std::vector<std::pair<DMapFile::dmapElem, ptrmapFile> >::iterator iter_p, iter_n;
  bool ret = true;

  dmap_err.clear();
  map_err.clear();
  if (dmaps.size() < 2) {
    return true;
  }

  /* FIXME: why not use dmapFile.check instead of repeating here */
  std::sort(dmaps.begin(), dmaps.end(), copmaredMapElemsByName_functor());
  iter_p = dmaps.begin();
  iter_n = iter_p + 1;
  while (1) {
    if ((*iter_p).first.dev_name == (*iter_n).first.dev_name) {
      if ((*iter_p).first.dev_file != (*iter_n).first.dev_file || (*iter_p).first.map_file_name != (*iter_n).first.map_file_name) {
	dmap_err.insert(DMapFile::errorList::errorElem(DMapFile::errorList::errorElem::ERROR, DMapFile::errorList::errorElem::NONUNIQUE_DEVICE_NAME, (*iter_p).first, (*iter_n).first));
	ret = false;
      }
    }
    iter_p = iter_n;
    iter_n = ++iter_n;
    if (iter_n == dmaps.end()) {
      break;
    }
  }
  /* Why not pass map_err by reference? because we are clearing stuff inside
   * check.. why???
   * */
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

ptrmapFile DMapFilesParser::getMapFile(const std::string &dev_name) {
  std::vector<std::pair<DMapFile::dmapElem, ptrmapFile> >::iterator dmap_iter;
  dmap_iter = std::find_if(dmap_elems.begin(), dmap_elems.end(), findDevInPairByName_pred(dev_name));
  if (dmap_iter == dmap_elems.end()) {
    throw DMapFileParserException("Cannot find device " + dev_name, LibMapException::EX_NO_DEVICE_IN_DMAP_FILE);
  }
  return (*dmap_iter).second;
}

void DMapFilesParser::getdMapFileElem(const std::string &dev_name, DMapFile::dmapElem &dMapFileElem) {
  dMapFileElem = getdMapFileElem( dev_name );
}

DMapFile::dmapElem const & DMapFilesParser::getdMapFileElem(const std::string &dev_name) {
  std::vector<std::pair<DMapFile::dmapElem, ptrmapFile> >::iterator dmap_iter;
  dmap_iter = std::find_if(dmap_elems.begin(), dmap_elems.end(), findDevInPairByName_pred(dev_name));
  if (dmap_iter == dmap_elems.end()) {
    throw DMapFileParserException("Cannot find device " + dev_name, LibMapException::EX_NO_DEVICE_IN_DMAP_FILE);
  }
  return (*dmap_iter).first;
}

void DMapFilesParser::getdMapFileElem(int elem_nr, DMapFile::dmapElem &dMapFileElem) {
  try {
    dMapFileElem = dmap_elems.at(elem_nr).first;
  } catch (std::out_of_range) {
    throw DMapFileParserException("Cannot find device", LibMapException::EX_NO_DEVICE_IN_DMAP_FILE);
  }
}

void DMapFilesParser::getRegisterInfo(std::string dev_name, const std::string &reg_name, std::string& dev_file, uint32_t& reg_elem_nr, uint32_t& reg_offset, uint32_t& reg_size, uint32_t& reg_bar) {
  std::vector<std::pair<DMapFile::dmapElem, ptrmapFile> >::iterator dmap_iter;
  mapFile::mapElem elem;

  if (dev_name == "" && dmap_elems.size() == 1) {
    dev_name = dmap_elems[0].first.dev_name;
  }
  dmap_iter = std::find_if(dmap_elems.begin(), dmap_elems.end(), findDevInPairByName_pred(dev_name));
  if (dmap_iter == dmap_elems.end()) {
    throw DMapFileParserException("Cannot find device " + dev_name, LibMapException::EX_NO_DEVICE_IN_DMAP_FILE);
  }
  (*dmap_iter).second->getRegisterInfo(reg_name, elem);
  reg_offset = elem.reg_address;
  reg_size = elem.reg_size;
  reg_elem_nr = elem.reg_elem_nr;
  reg_bar = elem.reg_bar;
  dev_file = (*dmap_iter).first.dev_file;
}

void DMapFilesParser::getRegisterInfo(std::string dev_name, const std::string &reg_name, std::string& dev_file, mapFile::mapElem &elem) {
  std::vector<std::pair<DMapFile::dmapElem, ptrmapFile> >::iterator dmap_iter;

  if (dev_name == "" && dmap_elems.size() == 1) {
    dev_name = dmap_elems[0].first.dev_name;
  }
  dmap_iter = std::find_if(dmap_elems.begin(), dmap_elems.end(), findDevInPairByName_pred(dev_name));
  if (dmap_iter == dmap_elems.end()) {
    throw DMapFileParserException("Cannot find device " + dev_name, LibMapException::EX_NO_DEVICE_IN_DMAP_FILE);
  }
  (*dmap_iter).second->getRegisterInfo(reg_name, elem);
  dev_file = (*dmap_iter).first.dev_file;
}

std::ostream& operator<<(std::ostream &os, const DMapFilesParser& dmfp) {
  std::vector<std::pair<DMapFile::dmapElem, ptrmapFile> >::const_iterator iter;
  for (iter = dmfp.dmap_elems.begin(); iter != dmfp.dmap_elems.end(); ++iter) {
    os << (*iter).first << std::endl;
  }
  return os;
}

void DMapFilesParser::cleanAll() {
  map_files.clear();
  dmap_elems.clear();
}

DMapFilesParser::~DMapFilesParser() {
  cleanAll();
}

uint16_t DMapFilesParser::getdMapFileSize() {
  return dmap_elems.size();
}

DMapFilesParser::DMapFilesParser()
{
}

DMapFilesParser::DMapFilesParser(const std::string &dir) {
  parse_dir(dir);
}


DMapFilesParser::iterator DMapFilesParser::begin() {
  return dmap_elems.begin();
}

DMapFilesParser::const_iterator DMapFilesParser::begin() const{
  return dmap_elems.begin();
}

DMapFilesParser::iterator DMapFilesParser::end() {
  return dmap_elems.end();
}

DMapFilesParser::const_iterator DMapFilesParser::end() const{
  return dmap_elems.end();
}

std::string DMapFilesParser::getCurrentWorkingDirectory() {
  char *currentWorkingDir = get_current_dir_name();
  if (!currentWorkingDir) {
    throw;
  }
  std::string dir(currentWorkingDir);
  free(currentWorkingDir);
  return dir;
}

void DMapFilesParser::combinePaths(std::string &absoluteBasePath,
                                   const std::string &pathToAppend) {
  /* pathToAppend can only be empty if a dmap file in the root directory has been requested.
   * In this case the search for '/' has returned pos=0, resulting in an empty pathToAppend.
   */
  if(pathToAppend.empty()){
    absoluteBasePath = "/";
  }

  if (pathToAppend[0] == '/') {// absolute path, replace the base path
    absoluteBasePath = pathToAppend;
  } else {// relative path
    absoluteBasePath = absoluteBasePath + '/' + pathToAppend;
  }
}

} //namespace mtca4u


