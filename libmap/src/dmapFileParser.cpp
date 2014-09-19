#include "dmapFileParser.h"
#include "exlibmap.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <fstream>

namespace mtca4u{

ptrdmapFile dmapFileParser::parse(const std::string &file_name) {
    std::ifstream file;
    std::string line;
    std::istringstream is;
    dmapFile::dmapElem de;
    uint32_t line_nr = 0;
    std::string critical;

    file.open(file_name.c_str());
    if (!file) {        
        throw exDmapFileParser("Cannot open dmap file: \"" + file_name + "\"", exLibMap::EX_CANNOT_OPEN_DMAP_FILE);
    }

    ptrdmapFile dmap(new dmapFile(file_name));
    while (std::getline(file, line)) {
        line_nr++;
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), std::not1(std::ptr_fun<int, int>(isspace))));
        if (!line.size()) {
            continue;
        }
        if (line[0] == '#') {
            continue;
        }
        is.str(line);
        is >> de.dev_name >> de.dev_file >> de.map_file_name;                        
        if (is) {
            de.dmap_file_name = file_name;
            de.dmap_file_line_nr = line_nr; 
            dmap->insert(de);
        } else {
            std::ostringstream os;
            os << line_nr;
            throw exDmapFileParser("Error in dmap file: \"" + file_name + "\" in line (" + os.str() + ") \"" + line + "\"", exLibMap::EX_DMAP_FILE_PARSE_ERROR);
        }       
        is.clear();
    }
    file.close();
    if (dmap->getdmapFileSize() == 0) {
        throw exDmapFileParser("No data in in dmap file: \"" + file_name + "\"", exLibMap::EX_NO_DMAP_DATA);
    }
    return dmap;
}

}//namespace mtca4u
