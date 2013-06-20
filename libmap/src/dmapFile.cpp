#include "dmapFile.h"
#include "predicates.h"
#include "exlibmap.h"


#include <algorithm>

dmapFile::dmapFile(const std::string &file_name)
: dmap_file_name(file_name) {
}

size_t dmapFile::getdmapFileSize() {
    return dmap_file_elems.size();
}

std::ostream& operator<<(std::ostream &os, const dmapFile& me) {
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

void dmapFile::insert(const dmapElem &elem) {
    dmap_file_elems.push_back(elem);
}

void dmapFile::getDeviceInfo(const std::string& dev_name, dmapElem &value) {
    std::vector<dmapElem>::iterator iter;
    iter = find_if(dmap_file_elems.begin(), dmap_file_elems.end(), findDevByName_pred(dev_name));
    if (iter == dmap_file_elems.end()) {
        throw exDmapFile("Cannot find device \"" + dev_name + "\"in DMAP file", exLibMap::EX_NO_DEVICE_IN_DMAP_FILE);
    }
    value = *iter;
}

dmapFile::dmapElem::dmapElem() : dmap_file_line_nr(0)
{
}        

std::ostream& operator<<(std::ostream &os, const dmapFile::dmapElem& de) {
    os << "(" << de.dmap_file_name << ") NAME: " << de.dev_name << " DEV : " << de.dev_file << " MAP : " << de.map_file_name;
    return os;
}

bool dmapFile::check(dmapFile::errorList &err, dmapFile::errorList::errorElem::TYPE level) {

    std::vector<dmapFile::dmapElem> dmaps = dmap_file_elems;
    std::vector<dmapFile::dmapElem>::iterator iter_p, iter_n;
    bool ret = true;

    err.clear();
    if (dmaps.size() < 2) {
        return true;
    }

    std::sort(dmaps.begin(), dmaps.end(), copmaredMapElemsByName2_functor());
    iter_p = dmaps.begin();
    iter_n = iter_p + 1;
    while (1) {
        if ((*iter_p).dev_name == (*iter_n).dev_name) {
            if ((*iter_p).dev_file != (*iter_n).dev_file || (*iter_p).map_file_name != (*iter_n).map_file_name) {
                err.insert(errorList::errorElem(errorList::errorElem::ERROR, errorList::errorElem::NONUNIQUE_DEVICE_NAME, (*iter_p), (*iter_n)));
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



std::ostream& operator<<(std::ostream &os, const dmapFile::errorList::errorElem::TYPE& me) {
    switch (me) {
        case dmapFile::errorList::errorElem::ERROR:
            os << "ERROR";
            break;
        case dmapFile::errorList::errorElem::WARNING:
            os << "WARNING";
            break;
        default:
            os << "UNKNOWN";
            break;
    }
    return os;
}

dmapFile::errorList::errorElem::errorElem(dmapFile::errorList::errorElem::TYPE info_type, dmapFile::errorList::errorElem::DMAP_FILE_ERR e_type, const dmapFile::dmapElem &dev_1, const dmapFile::dmapElem &dev_2) {
    err_type = e_type;
    err_dev_1 = dev_1;
    err_dev_2 = dev_2;
    type = info_type;
}

std::ostream& operator<<(std::ostream &os, const dmapFile::errorList::errorElem& me) {
    switch (me.err_type) {
        case dmapFile::errorList::errorElem::NONUNIQUE_DEVICE_NAME:
            os << me.type << ": Found two devices with the same name but different properties: \"" << me.err_dev_1.dev_name << "\" in file \"" << me.err_dev_1.dmap_file_name << "\" in line " << me.err_dev_1.dmap_file_line_nr << " and \"" << me.err_dev_2.dmap_file_name << "\" in line " << me.err_dev_2.dmap_file_line_nr;
            break;
    }
    return os;
}

void dmapFile::errorList::clear() {
    errors.clear();
}

void dmapFile::errorList::insert(const errorElem& elem) {
    errors.push_back(elem);
}

std::ostream& operator<<(std::ostream &os, const dmapFile::errorList& me) {
    std::list<dmapFile::errorList::errorElem>::const_iterator iter;
    for (iter = me.errors.begin(); iter != me.errors.end(); iter++) {
        os << (*iter) << std::endl;
    }
    return os;
}

dmapFile::iterator dmapFile::begin() {
    return iterator(dmap_file_elems.begin());
}

dmapFile::iterator dmapFile::end() {
    return iterator(dmap_file_elems.end());
}
