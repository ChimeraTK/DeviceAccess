#include "MapFile.h"
#include "predicates.h"
#include "ExceptionMap.h"
#include <algorithm>
#include <stdexcept>

namespace mtca4u{

std::ostream& operator<<(std::ostream &os, const mapFile& me) {
    size_t size;
    os << "=======================================" << std::endl;
    os << "MAP FILE NAME: " << me.map_file_name << std::endl;
    os << "---------------------------------------" << std::endl;
    for (size = 0; size < me.metadata.size(); size++) {
        os << me.metadata[size];
    }
    os << "---------------------------------------" << std::endl;
    for (size = 0; size < me.map_file_elems.size(); size++) {
        os << me.map_file_elems[size] << std::endl;
    }
    os << "=======================================";
    return os;
}

void mapFile::insert(mapElem &elem) {
    map_file_elems.push_back(elem);
}

void mapFile::insert(metaData &elem) {
    metadata.push_back(elem);
}

  void mapFile::getRegisterInfo(const std::string& reg_name, mapElem &value,
				const std::string& reg_module) const{
    std::vector<mapElem>::const_iterator iter;
    iter = std::find_if(map_file_elems.begin(), map_file_elems.end(), 
			findRegisterByName_pred(reg_name, reg_module));
    if (iter == map_file_elems.end()) {
        throw exMapFile("Cannot find register " + reg_module + (reg_module.empty()?"":".") + reg_name +
			" in map file: " + map_file_name, exLibMap::EX_NO_REGISTER_IN_MAP_FILE);
    }
    value = *iter;
}

void mapFile::getRegisterInfo(int reg_nr, mapElem &value) const {
    try {
        value = map_file_elems.at(reg_nr);
    } catch (std::out_of_range) {
        throw exMapFile("Cannot find register in map file", exLibMap::EX_NO_REGISTER_IN_MAP_FILE);
    }
}

void mapFile::getMetaData(const std::string &metaDataName, std::string& metaDataValue) const{
    std::vector<mapFile::metaData>::const_iterator iter;

    iter = std::find_if(metadata.begin(), metadata.end(), findMetaDataByName_pred(metaDataName));
    if (iter == metadata.end()) {
        throw exMapFile("Cannot find metadata " + metaDataName + " in map file: " + map_file_name, exLibMap::EX_NO_METADATA_IN_MAP_FILE);
    }
    metaDataValue = (*iter).value;
}

namespace {

    typedef struct _addresses {
        uint32_t start;
        uint32_t end;
        std::vector<mapFile::mapElem>::iterator iter;
    } addresses;

}

bool mapFile::check(errorList &err, mapFile::errorList::errorElem::TYPE level) {
    std::vector<addresses> v_addresses;
    std::vector<addresses>::iterator v_iter;
    addresses address;

    std::vector<mapElem> map_file = map_file_elems;
    std::vector<mapElem>::iterator iter_p, iter_n;
    bool ret = true;

    err.clear();
    if (map_file_elems.size() < 2)
        return true;
    std::sort(map_file.begin(), map_file.end(), compareMapElemsByName_functor());
    iter_p = map_file.begin();
    iter_n = iter_p + 1;
    while (1) {
        if ( (iter_p->reg_name == iter_n->reg_name) && (iter_p->reg_module == iter_n->reg_module) ) {
	    err.insert(errorList::errorElem(errorList::errorElem::ERROR, 
					    errorList::errorElem::NONUNIQUE_REGISTER_NAME,
					    *iter_p, *iter_n, map_file_name));
	    ret = false;
	}
	iter_p = iter_n;
	iter_n = ++iter_n;
	if (iter_n == map_file.end()) {
	    break;
	}
    }

    for (iter_p = map_file.begin(); iter_p != map_file.end(); iter_p++) {
        address.start = iter_p->reg_address;
        address.end = iter_p->reg_address + iter_p->reg_size;
        address.iter = iter_p;
        for (v_iter = v_addresses.begin(); v_iter != v_addresses.end(); v_iter++) {
            // only addresses in the same module are considered to overlap
            if (iter_p->reg_module != v_iter->iter->reg_module){
                continue;
	    }

            if (iter_p->reg_bar == (v_iter->iter)->reg_bar) {
	        if ( (v_iter->start >= iter_p->reg_address) && 
		     (v_iter->start < (iter_p->reg_address + iter_p->reg_size)) ) {
		    if (level >= errorList::errorElem::WARNING){
                        err.insert(errorList::errorElem(errorList::errorElem::WARNING,
							errorList::errorElem::WRONG_REGISTER_ADDRESSES,
							*iter_p, *(v_iter->iter), map_file_name));
                        ret = false;
                    }
                } else if ( (v_iter->start <= iter_p->reg_address) && (iter_p->reg_address < v_iter->end) ) {
                    if (level >= errorList::errorElem::WARNING){
                        err.insert(errorList::errorElem(errorList::errorElem::WARNING,
							errorList::errorElem::WRONG_REGISTER_ADDRESSES,
							*iter_p, *(v_iter->iter), map_file_name));
                        ret = false;
                    }
                }
            }
        }
        v_addresses.push_back(address);
    }
    return ret;
}

std::ostream& operator<<(std::ostream &os, const mapFile::metaData& me) {
    os << "METADATA-> NAME: \"" << me.name << "\" VALUE: " << me.value << std::endl;
    return os;
}

std::ostream& operator<<(std::ostream &os, const mapFile::mapElem& me) {
     os << me.reg_name << " 0x" << std::hex << me.reg_elem_nr << " 0x" << me.reg_address 
	<< " 0x" << me.reg_size << " 0x" << me.reg_bar << std::dec
	<< " " << me.reg_width << " " << me.reg_frac_bits << " " 
	<< (me.reg_signed?"true":"false" ) 
	<< (!me.reg_module.empty()?" "+me.reg_module:"");
    return os;
}

std::ostream& operator<<(std::ostream &os, const mapFile::errorList::errorElem::TYPE& me) {
    switch (me) {
        case mapFile::errorList::errorElem::ERROR:
            os << "ERROR";
            break;
        case mapFile::errorList::errorElem::WARNING:
            os << "WARNING";
            break;
        default:
            os << "UNKNOWN";
            break;
    }
    return os;
}

mapFile::errorList::errorElem::errorElem(mapFile::errorList::errorElem::TYPE info_type, mapFile::errorList::errorElem::MAP_FILE_ERR e_type, const mapFile::mapElem &reg_1, const mapFile::mapElem &reg_2, const std::string &file_name) {
    err_type = e_type;
    err_reg_1 = reg_1;
    err_reg_2 = reg_2;
    err_file_name = file_name;
    type = info_type;
}

std::ostream& operator<<(std::ostream &os, const mapFile::errorList::errorElem& me) {
    switch (me.err_type) {
        case mapFile::errorList::errorElem::NONUNIQUE_REGISTER_NAME:
            os << me.type << ": Found two registers with the same name: \"" << me.err_reg_1.reg_name << "\" in file " << me.err_file_name << " in lines " << me.err_reg_1.line_nr << " and " << me.err_reg_2.line_nr;
            break;
        case mapFile::errorList::errorElem::WRONG_REGISTER_ADDRESSES:
            os << me.type << ": Found two registers with overlapping addresses: \"" << me.err_reg_1.reg_name << "\" and \"" << me.err_reg_2.reg_name << "\" in file " << me.err_file_name << " in lines " << me.err_reg_1.line_nr << " and " << me.err_reg_2.line_nr;
            break;
    }
    return os;
}

void mapFile::errorList::clear() {
    errors.clear();
}

void mapFile::errorList::insert(const mapFile::errorList::errorElem& elem) {
    errors.push_back(elem);
}

std::ostream& operator<<(std::ostream &os, const mapFile::errorList& me) {
    std::list<mapFile::errorList::errorElem>::const_iterator iter;
    for (iter = me.errors.begin(); iter != me.errors.end(); iter++) {
        os << *iter << std::endl;
    }
    return os;
}

mapFile::mapFile(const std::string &file_name)
: map_file_name(file_name) {
}

const std::string& mapFile::getMapFileName() const {
    return map_file_name;
}

size_t mapFile::getMapFileSize() const {
    return map_file_elems.size();
}

mapFile::iterator mapFile::begin() {
    return map_file_elems.begin();
}

mapFile::const_iterator mapFile::begin() const{
    return map_file_elems.begin();
}

mapFile::const_iterator mapFile::end() const{
    return map_file_elems.end();
}

mapFile::iterator mapFile::end(){
    return map_file_elems.end();
}

std::list< mapFile::mapElem > mapFile::getRegistersInModule( std::string const & moduleName){
  // first sort all elements accordind the names (module first, then register in module)
  // make a copy to keep the original order from the map file
  std::vector<mapElem> sortedMapElements = map_file_elems;
  std::sort(sortedMapElements.begin(), sortedMapElements.end(), compareMapElemsByName_functor());
  
  // The vector is sorted, first module, than register name.
  // Find the first iterator with the module name, starting at the beginning,
  // then search for the first iterator with another name, starting from the previousy found first match.
  std::vector<mapElem>::iterator firstMatchingIterator = 
      std::find_if( sortedMapElements.begin(), sortedMapElements.end(), compareModuleName_pred( moduleName ) );
  std::vector<mapElem>::iterator firstNotMatchingIterator = 
      std::find_if( firstMatchingIterator, sortedMapElements.end(),
		    std::not1( compareModuleName_pred(moduleName) ) );

  // fill the list
  std::list<mapElem> mapElementList;
  for (std::vector<mapElem>::iterator it = firstMatchingIterator;
       it != firstNotMatchingIterator; ++it){
    mapElementList.push_back( *it );
  }

  return mapElementList;
}

mapFile::metaData::metaData( std::string const & the_name,
			    std::string const & the_value)
  : name(the_name), value(the_value)
{}

mapFile::mapElem::mapElem( std::string const & the_reg_name,
			   uint32_t the_reg_elem_nr,
			   uint32_t the_reg_address,
			   uint32_t the_reg_size,
			   uint32_t the_reg_bar,
			   uint32_t the_reg_width,
			   int32_t  the_reg_frac_bits,
			   bool     the_reg_signed,
			   uint32_t the_line_nr,
			   std::string const & the_reg_module)
  : reg_name( the_reg_name ),  reg_elem_nr(the_reg_elem_nr), reg_address(the_reg_address),
    reg_size(the_reg_size), reg_bar(the_reg_bar), reg_width(the_reg_width), reg_frac_bits(the_reg_frac_bits), 
    reg_signed(the_reg_signed), line_nr(the_line_nr), reg_module(the_reg_module)
{}

}//namespace mtca4u
