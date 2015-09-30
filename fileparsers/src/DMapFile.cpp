#include "predicates.h"
#include "MapException.h"
#include "DMapFile.h"
#include <algorithm>

namespace mtca4u{

DMapFile::DMapFile(const std::string &file_name)
: dmap_file_name(file_name) {
}

size_t DMapFile::getdmapFileSize() {
	return dmap_file_elems.size();
}

std::ostream& operator<<(std::ostream &os, const DMapFile& me) {
	size_t size;
	os << "=======================================" << std::endl;
	os << "MAP FILE NAME: " << me.dmap_file_name << std::endl;
	os << "---------------------------------------" << std::endl;
	for (size = 0; size < me.dmap_file_elems.size(); size++) {
		os << me.dmap_file_elems[size] << std::endl;
	}
	os << "=======================================";
	return os;
}

void DMapFile::insert(const DRegisterInfo &elem) {
	dmap_file_elems.push_back(elem);
}

void DMapFile::getDeviceInfo(const std::string& dev_name, DRegisterInfo &value) {
	std::vector<DRegisterInfo>::iterator iter;
	iter = find_if(dmap_file_elems.begin(), dmap_file_elems.end(), findDevByName_pred(dev_name));
	if (iter == dmap_file_elems.end()) {
		throw DMapFileException("Cannot find device \"" + dev_name + "\"in DMAP file", LibMapException::EX_NO_DEVICE_IN_DMAP_FILE);
	}
	value = *iter;
}

DMapFile::DRegisterInfo::DRegisterInfo() : dmap_file_line_nr(0)
{
}        

std::pair<std::string, std::string> DMapFile::DRegisterInfo::getDeviceFileAndMapFileName() const
{
	return std::pair<std::string, std::string>(dev_file, map_file_name);
}        

std::ostream& operator<<(std::ostream &os, const DMapFile::DRegisterInfo& de) {
	os << "(" << de.dmap_file_name << ") NAME: " << de.dev_name << " DEV : " << de.dev_file << " MAP : " << de.map_file_name;
	return os;
}

//fixme: why is level not used?
bool DMapFile::check(DMapFile::ErrorList &err, DMapFile::ErrorList::ErrorElem::TYPE /*level*/) {

	std::vector<DMapFile::DRegisterInfo> dmaps = dmap_file_elems;
	std::vector<DMapFile::DRegisterInfo>::iterator iter_p, iter_n;
	bool ret = true;

	err.clear();
	if (dmaps.size() < 2) {
		return true;
	}

	std::sort(dmaps.begin(), dmaps.end(), copmaredRegisterInfosByName2_functor());
	iter_p = dmaps.begin();
	iter_n = iter_p + 1;
	while (1) {
		if ((*iter_p).dev_name == (*iter_n).dev_name) {
			if ((*iter_p).dev_file != (*iter_n).dev_file || (*iter_p).map_file_name != (*iter_n).map_file_name) {
				err.insert(ErrorList::ErrorElem(ErrorList::ErrorElem::ERROR, ErrorList::ErrorElem::NONUNIQUE_DEVICE_NAME, (*iter_p), (*iter_n)));
				ret = false;
			}
		}
		iter_p = iter_n;
		iter_n = ++iter_n;
		if (iter_n == dmaps.end()) {
			break;
		}
	}
	return ret;
}

std::ostream& operator<<(std::ostream &os, const DMapFile::ErrorList::ErrorElem::TYPE& me) {
	switch (me) {
	case DMapFile::ErrorList::ErrorElem::ERROR:
		os << "ERROR";
		break;
	case DMapFile::ErrorList::ErrorElem::WARNING:
		os << "WARNING";
		break;
	default:
		os << "UNKNOWN";
		break;
	}
	return os;
}

DMapFile::ErrorList::ErrorElem::ErrorElem(DMapFile::ErrorList::ErrorElem::TYPE info_type, DMapFile::ErrorList::ErrorElem::DMAP_FILE_ERR e_type, const DMapFile::DRegisterInfo &dev_1, const DMapFile::DRegisterInfo &dev_2) {
	err_type = e_type;
	err_dev_1 = dev_1;
	err_dev_2 = dev_2;
	type = info_type;
}

std::ostream& operator<<(std::ostream &os, const DMapFile::ErrorList::ErrorElem& me) {
	switch (me.err_type) {
	case DMapFile::ErrorList::ErrorElem::NONUNIQUE_DEVICE_NAME:
		os << me.type << ": Found two devices with the same name but different properties: \"" << me.err_dev_1.dev_name << "\" in file \"" << me.err_dev_1.dmap_file_name << "\" in line " << me.err_dev_1.dmap_file_line_nr << " and \"" << me.err_dev_2.dmap_file_name << "\" in line " << me.err_dev_2.dmap_file_line_nr;
		break;
	}
	return os;
}

void DMapFile::ErrorList::clear() {
	errors.clear();
}

void DMapFile::ErrorList::insert(const ErrorElem& elem) {
	errors.push_back(elem);
}

std::ostream& operator<<(std::ostream &os, const DMapFile::ErrorList& me) {
	std::list<DMapFile::ErrorList::ErrorElem>::const_iterator iter;
	for (iter = me.errors.begin(); iter != me.errors.end(); ++iter) {
		os << (*iter) << std::endl;
	}
	return os;
}

DMapFile::iterator DMapFile::begin() {
	return dmap_file_elems.begin();
}

DMapFile::iterator DMapFile::end() {
	return dmap_file_elems.end();
}

}//namespace mtca4u
