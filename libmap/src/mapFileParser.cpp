#include "mapFileParser.h"
#include "exlibmap.h"
#include <iostream>
#include <algorithm>
#include <sstream>

namespace mtca4u{

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
        bool failed = false;
        line_nr++;
        // Remove whitespace from beginning of line
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), std::not1(std::ptr_fun<int,int>(isspace))));
        if (!line.size())       {continue;}
        if (line[0] == '#')     {continue;}
        if (line[0] == '@'){
            std::string org_line = line;
            mapFile::metaData md;
            // Remove the '@' character...
            line.erase(line.begin(), line.begin() + 1);
            // ... and remove all the whitespace after it        
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
        // first, set default values for 'optional' fields
        me.reg_bar = 0xFFFFFFFF;
        me.reg_width= 32;
        me.reg_frac_bits = 0;
        me.reg_signed = true;
        is >> std::setbase(0) >> me.reg_bar;         
        if (is.fail()){
          failed = true;
        }      
        if (!failed) {
          is >> std::setbase(0) >> me.reg_width;         
          if (is.fail()){
            failed = true;
          } else {
            if (me.reg_width > 32) {
              std::ostringstream os;
              os << line_nr;
              throw exMapFileParser("Error in map file (register width too big): \"" + file_name + "\" in line (" + os.str() + ") \"" + line + "\"", exLibMap::EX_MAP_FILE_PARSE_ERROR);
            }
          }
        }
        if (!failed) {
          is >> std::setbase(0) >> me.reg_frac_bits;         
          if (is.fail()){
            failed = true;
          } else {
            if (me.reg_frac_bits > 1023 || me.reg_frac_bits < -1024) {
              std::ostringstream os;
              os << line_nr;
              throw exMapFileParser("Error in map file (too many fractional bits): \"" + file_name + "\" in line (" + os.str() + ") \"" + line + "\"", exLibMap::EX_MAP_FILE_PARSE_ERROR);
            }
          }
        }
  
        if (!failed) {
          is >> std::setbase(0) >> me.reg_signed;         
          if (is.fail()){
            failed = true;
          }
        }
        is.clear();
        me.line_nr = line_nr;
        pmap->insert(me);
    }
    return pmap;
}

}//namespace mtca4u

