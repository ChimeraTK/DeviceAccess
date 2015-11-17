#include "RegisterInfoMap.h"
#include "predicates.h"
#include "MapException.h"
#include <algorithm>
#include <stdexcept>

namespace mtca4u{

std::ostream& operator<<(std::ostream &os, const RegisterInfoMap& registerInfoMap) {
	size_t size;
	os << "=======================================" << std::endl;
	os << "MAP FILE NAME: " << registerInfoMap._mapFileName << std::endl;
	os << "---------------------------------------" << std::endl;
	for (size = 0; size < registerInfoMap._metadata.size(); size++) {
		os << registerInfoMap._metadata[size];
	}
	os << "---------------------------------------" << std::endl;
	for (size = 0; size < registerInfoMap._mapFileElements.size(); size++) {
		os << registerInfoMap._mapFileElements[size] << std::endl;
	}
	os << "=======================================";
	return os;
}

void RegisterInfoMap::insert(RegisterInfo &elem) {
	_mapFileElements.push_back(elem);
}

void RegisterInfoMap::insert(MetaData &elem) {
	_metadata.push_back(elem);
}

void RegisterInfoMap::getRegisterInfo(const std::string& reg_name, RegisterInfo &value,
		const std::string& reg_module) const{
	std::vector<RegisterInfo>::const_iterator iter;
	iter = std::find_if(_mapFileElements.begin(), _mapFileElements.end(),
			findRegisterByName_pred(reg_name, reg_module));
	if (iter == _mapFileElements.end()) {
		throw MapFileException("Cannot find register " + reg_module + (reg_module.empty()?"":".") + reg_name +
				" in map file: " + _mapFileName, LibMapException::EX_NO_REGISTER_IN_MAP_FILE);
	}
	value = *iter;
}

void RegisterInfoMap::getRegisterInfo(int reg_nr, RegisterInfo &value) const {
	try {
		value = _mapFileElements.at(reg_nr);
	} catch (std::out_of_range) {
		throw MapFileException("Cannot find register in map file", LibMapException::EX_NO_REGISTER_IN_MAP_FILE);
	}
}

void RegisterInfoMap::getMetaData(const std::string &metaDataName, std::string& metaDataValue) const{
	std::vector<RegisterInfoMap::MetaData>::const_iterator iter;

	iter = std::find_if(_metadata.begin(), _metadata.end(), findMetaDataByName_pred(metaDataName));
	if (iter == _metadata.end()) {
		throw MapFileException("Cannot find metadata " + metaDataName + " in map file: " + _mapFileName, LibMapException::EX_NO_METADATA_IN_MAP_FILE);
	}
	metaDataValue = (*iter)._value;
}

namespace {

typedef struct _addresses {
	uint32_t start;
	uint32_t end;
	std::vector<RegisterInfoMap::RegisterInfo>::iterator iter;
} addresses;

}

bool RegisterInfoMap::check(ErrorList &err, RegisterInfoMap::ErrorList::ErrorElem::TYPE level) {
	std::vector<addresses> v_addresses;
	std::vector<addresses>::iterator v_iter;
	addresses address;

	std::vector<RegisterInfo> map_file = _mapFileElements;
	std::vector<RegisterInfo>::iterator iter_p, iter_n;
	bool ret = true;

	err.clear();
	if (_mapFileElements.size() < 2)
		return true;
	std::sort(map_file.begin(), map_file.end(), compareRegisterInfosByName_functor());
	iter_p = map_file.begin();
	iter_n = iter_p + 1;
	while (1) {
		if ( (iter_p->_name == iter_n->_name) && (iter_p->_module == iter_n->_module) ) {
			err.insert(ErrorList::ErrorElem(ErrorList::ErrorElem::ERROR,
					ErrorList::ErrorElem::NONUNIQUE_REGISTER_NAME,
					*iter_p, *iter_n, _mapFileName));
			ret = false;
		}
		iter_p = iter_n;
		iter_n = ++iter_n;
		if (iter_n == map_file.end()) {
			break;
		}
	}

	for (iter_p = map_file.begin(); iter_p != map_file.end(); iter_p++) {
		address.start = iter_p->_addressOffset;
		address.end = iter_p->_addressOffset + iter_p->_size;
		address.iter = iter_p;
		for (v_iter = v_addresses.begin(); v_iter != v_addresses.end(); v_iter++) {
			// only addresses in the same module are considered to overlap
			if (iter_p->_module != v_iter->iter->_module){
				continue;
			}

			if (iter_p->_bar == (v_iter->iter)->_bar) {
				if ( (v_iter->start >= iter_p->_addressOffset) &&
						(v_iter->start < (iter_p->_addressOffset + iter_p->_size)) ) {
					if (level >= ErrorList::ErrorElem::WARNING){
						err.insert(ErrorList::ErrorElem(ErrorList::ErrorElem::WARNING,
								ErrorList::ErrorElem::WRONG_REGISTER_ADDRESSES,
								*iter_p, *(v_iter->iter), _mapFileName));
						ret = false;
					}
				} else if ( (v_iter->start <= iter_p->_addressOffset) && (iter_p->_addressOffset < v_iter->end) ) {
					if (level >= ErrorList::ErrorElem::WARNING){
						err.insert(ErrorList::ErrorElem(ErrorList::ErrorElem::WARNING,
								ErrorList::ErrorElem::WRONG_REGISTER_ADDRESSES,
								*iter_p, *(v_iter->iter), _mapFileName));
						ret = false;
					}
				}
			}
		}
		v_addresses.push_back(address);
	}
	return ret;
}

std::ostream& operator<<(std::ostream &os, const RegisterInfoMap::MetaData& me) {
	os << "METADATA-> NAME: \"" << me._name << "\" VALUE: " << me._value << std::endl;
	return os;
}

std::ostream& operator<<(std::ostream &os, const RegisterInfoMap::RegisterInfo& registerInfo) {
	os << registerInfo._name << " 0x" << std::hex << registerInfo._elementCount << " 0x" << registerInfo._addressOffset
			<< " 0x" << registerInfo._size << " 0x" << registerInfo._bar << std::dec
			<< " " << registerInfo._width << " " << registerInfo._fractionalBits << " "
			<< (registerInfo._signedFlag?"true":"false" )
			<< (!registerInfo._module.empty()?" "+registerInfo._module:"");
	return os;
}

std::ostream& operator<<(std::ostream &os, const RegisterInfoMap::ErrorList::ErrorElem::TYPE& me) {
	switch (me) {
	case RegisterInfoMap::ErrorList::ErrorElem::ERROR:
		os << "ERROR";
		break;
	case RegisterInfoMap::ErrorList::ErrorElem::WARNING:
		os << "WARNING";
		break;
	default:
		os << "UNKNOWN";
		break;
	}
	return os;
}

RegisterInfoMap::ErrorList::ErrorElem::ErrorElem(RegisterInfoMap::ErrorList::ErrorElem::TYPE info_type, RegisterInfoMap::ErrorList::ErrorElem::MAP_FILE_ERR e_type, const RegisterInfoMap::RegisterInfo &reg_1, const RegisterInfoMap::RegisterInfo &reg_2, const std::string &file_name) {
	_errorType = e_type;
	_errorRegister1 = reg_1;
	_errorRegister2 = reg_2;
	_errorFileName = file_name;
	_type = info_type;
}

std::ostream& operator<<(std::ostream &os, const RegisterInfoMap::ErrorList::ErrorElem& me) {
	switch (me._errorType) {
	case RegisterInfoMap::ErrorList::ErrorElem::NONUNIQUE_REGISTER_NAME:
		os << me._type << ": Found two registers with the same name: \"" << me._errorRegister1._name << "\" in file " << me._errorFileName << " in lines " << me._errorRegister1._descriptionLineNumber << " and " << me._errorRegister2._descriptionLineNumber;
		break;
	case RegisterInfoMap::ErrorList::ErrorElem::WRONG_REGISTER_ADDRESSES:
		os << me._type << ": Found two registers with overlapping addresses: \"" << me._errorRegister1._name << "\" and \"" << me._errorRegister2._name << "\" in file " << me._errorFileName << " in lines " << me._errorRegister1._descriptionLineNumber << " and " << me._errorRegister2._descriptionLineNumber;
		break;
	}
	return os;
}

void RegisterInfoMap::ErrorList::clear() {
	errors.clear();
}

void RegisterInfoMap::ErrorList::insert(const RegisterInfoMap::ErrorList::ErrorElem& elem) {
	errors.push_back(elem);
}

std::ostream& operator<<(std::ostream &os, const RegisterInfoMap::ErrorList& me) {
	std::list<RegisterInfoMap::ErrorList::ErrorElem>::const_iterator iter;
	for (iter = me.errors.begin(); iter != me.errors.end(); iter++) {
		os << *iter << std::endl;
	}
	return os;
}

RegisterInfoMap::RegisterInfoMap()
{

}

RegisterInfoMap::RegisterInfoMap(const std::string &file_name)
: _mapFileName(file_name) {
}

const std::string& RegisterInfoMap::getMapFileName() const {
	return _mapFileName;
}

size_t RegisterInfoMap::getMapFileSize() const {
	return _mapFileElements.size();
}

RegisterInfoMap::iterator RegisterInfoMap::begin() {
	return _mapFileElements.begin();
}

RegisterInfoMap::const_iterator RegisterInfoMap::begin() const{
	return _mapFileElements.begin();
}

RegisterInfoMap::const_iterator RegisterInfoMap::end() const{
	return _mapFileElements.end();
}

RegisterInfoMap::iterator RegisterInfoMap::end(){
	return _mapFileElements.end();
}

std::list< RegisterInfoMap::RegisterInfo > RegisterInfoMap::getRegistersInModule( std::string const & moduleName){
	// first sort all elements accordind the names (module first, then register in module)
	// make a copy to keep the original order from the map file
	std::vector<RegisterInfo> sortedDeviceInfos = _mapFileElements;
	std::sort(sortedDeviceInfos.begin(), sortedDeviceInfos.end(), compareRegisterInfosByName_functor());

	// The vector is sorted, first module, than register name.
	// Find the first iterator with the module name, starting at the beginning,
	// then search for the first iterator with another name, starting from the previousy found first match.
	std::vector<RegisterInfo>::iterator firstMatchingIterator =
			std::find_if( sortedDeviceInfos.begin(), sortedDeviceInfos.end(), compareModuleName_pred( moduleName ) );
	std::vector<RegisterInfo>::iterator firstNotMatchingIterator =
			std::find_if( firstMatchingIterator, sortedDeviceInfos.end(),
					std::not1( compareModuleName_pred(moduleName) ) );

	// fill the list
	std::list<RegisterInfo> RegisterInfoentList;
	for (std::vector<RegisterInfo>::iterator it = firstMatchingIterator;
			it != firstNotMatchingIterator; ++it){
		RegisterInfoentList.push_back( *it );
	}

	return RegisterInfoentList;
}

RegisterInfoMap::MetaData::MetaData( std::string const & the_name,
		std::string const & the_value)
: _name(the_name), _value(the_value)
{}

RegisterInfoMap::RegisterInfo::RegisterInfo( std::string const & the_reg_name,
		uint32_t the_reg_elem_nr,
		uint32_t the_reg_address,
		uint32_t the_reg_size,
		uint32_t the_reg_bar,
		uint32_t the_reg_width,
		int32_t  the_reg_frac_bits,
		bool     the_reg_signed,
		uint32_t the_line_nr,
		std::string const & the_reg_module)
: _name( the_reg_name ),  _elementCount(the_reg_elem_nr), _addressOffset(the_reg_address),
	_size(the_reg_size), _bar(the_reg_bar), _width(the_reg_width), _fractionalBits(the_reg_frac_bits),
	_signedFlag(the_reg_signed), _descriptionLineNumber(the_line_nr), _module(the_reg_module)
{}

}//namespace mtca4u
