#include "MapException.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <fstream>
#include "DMapFileParser.h"

namespace mtca4u{

ptrdmapFile DMapFileParser::parse(const std::string &file_name) {
    std::ifstream file;
    std::string line;
    std::istringstream is;
    DMapFile::dRegisterInfo de;
    uint32_t line_nr = 0;
    //std::string critical;

    file.open(file_name.c_str());
    if (!file) {        
        throw DMapFileParserException("Cannot open dmap file: \"" + file_name + "\"", LibMapException::EX_CANNOT_OPEN_DMAP_FILE);
    }

    ptrdmapFile dmap(new DMapFile(file_name));
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
            throw DMapFileParserException("Error in dmap file: \"" + file_name + "\" in line (" + os.str() + ") \"" + line + "\"", LibMapException::EX_DMAP_FILE_PARSE_ERROR);
        }       
        is.clear();
    }
    file.close();
    if (dmap->getdmapFileSize() == 0) {
        throw DMapFileParserException("No data in in dmap file: \"" + file_name + "\"", LibMapException::EX_NO_DMAP_DATA);
    }
    return dmap;
}

}//namespace mtca4u
