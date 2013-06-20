#include "mapFileParser.h"
#include "exlibmap.h"
#include <iostream>
#include <algorithm>
#include <sstream>

ptrmapFile mapFileParser::parse(const std::string &file_name)
{
    std::ifstream           file;
    std::string             line;
    std::istringstream      is;
    uint32_t                line_nr = 0;
    
    file.open(file_name.c_str());
    if (!file){
        throw exMapFile("Cannot open file \"" + file_name + "\"", exLibMap::EX_CANNOT_OPEN_MAP_FILE);
    }
    ptrmapFile pmap(new mapFile(file_name));
    mapFile::mapElem me;

    while (std::getline(file, line)) {
        line_nr++;
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), std::not1(std::ptr_fun<int,int>(isspace))));
        if (!line.size())       {continue;}
        if (line[0] == '#')     {continue;}
        if (line[0] == '@'){
            std::string org_line = line;
            mapFile::metaData md;
            line.erase(line.begin(), line.begin() + 1);
            line.erase(line.begin(), std::find_if(line.begin(), line.end(), std::not1(std::ptr_fun<int,int>(isspace))));
            is.str(line);                        
            is >> md.name;   
            if (!is){
                std::ostringstream os;
                os << line_nr;
                throw exMapFileParser("Error in map file: \"" + file_name + "\" in line (" + os.str() + ") \"" + org_line + "\"", exLibMap::EX_MAP_FILE_PARSE_ERROR);
            }
            line.erase(line.begin(), line.begin() + md.name.length());
            line.erase(line.begin(), std::find_if(line.begin(), line.end(), std::not1(std::ptr_fun<int,int>(isspace))));
            md.value = line;
            pmap->insert(md);            
            is.clear();
            continue;
        }
        is.str(line);
        is >> me.reg_name >> std::setbase(0) >> me.reg_elem_nr >> std::setbase(0) >> me.reg_address >> std::setbase(0) >> me.reg_size;
        if (!is){
            std::ostringstream os;
            os << line_nr;
            throw exMapFileParser("Error in map file: \"" + file_name + "\" in line (" + os.str() + ") \"" + line + "\"", exLibMap::EX_MAP_FILE_PARSE_ERROR);
        }                        
        is >> std::setbase(0) >> me.reg_bar;         
        if (is.fail()){
                me.reg_bar = 0xFFFFFFFF;
        }         
        is.clear();
        me.line_nr = line_nr;
        pmap->insert(me);
    }
    return pmap;
}


