#include <iostream>
#include <algorithm>
#include <sstream>

#include "MapFileParser.h"
#include "MapException.h"
#include "NumericAddressedBackendMuxedRegisterAccessor.h"        // for the MULTIPLEXED_SEQUENCE_PREFIX constant

namespace ChimeraTK {

  RegisterInfoMapPointer MapFileParser::parse(const std::string &file_name)
  {
    std::ifstream           file;
    std::string             line;
    std::istringstream      is;
    uint32_t                line_nr = 0;

    file.open(file_name.c_str());
    if (!file){
      throw MapFileException("Cannot open file \"" + file_name + "\"", LibMapException::EX_CANNOT_OPEN_MAP_FILE);
    }
    RegisterInfoMapPointer pmap(new RegisterInfoMap(file_name));
    std::string name; /**< Name of register */
    uint32_t nElements; /**< Number of elements in register */
    uint32_t address; /**< Relative address in bytes from beginning  of the bar(Base Address Range)*/
    uint32_t nBytes; /**< Size of register expressed in bytes */
    uint32_t bar; /**< Number of bar with register */
    uint32_t width; /**< Number of significant bits in the register */
    int32_t  nFractionalBits; /**< Number of fractional bits */
    bool     signedFlag; /**< Signed/Unsigned flag */
    uint32_t lineNumber; /**< Number of line with description of register in MAP file */
    std::string module; /**< Name of the module this register is in*/

    while (std::getline(file, line)) {
      bool failed = false;
      line_nr++;
      // Remove whitespace from beginning of line
      line.erase(line.begin(), std::find_if(line.begin(), line.end(), std::not1(std::ptr_fun<int,int>(isspace))));
      if (!line.size())       {continue;}
      if (line[0] == '#')     {continue;}
      if (line[0] == '@'){
        std::string org_line = line;
        RegisterInfoMap::MetaData md;
        // Remove the '@' character...
        line.erase(line.begin(), line.begin() + 1);
        // ... and remove all the whitespace after it
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), std::not1(std::ptr_fun<int,int>(isspace))));
        is.str(line);
        is >> md.name;
        if (!is){
          std::ostringstream os;
          os << line_nr;
          throw MapFileParserException("Error in map file: \"" + file_name + "\" in line (" + os.str() + ") \"" + org_line + "\"", LibMapException::EX_MAP_FILE_PARSE_ERROR);
        }
        line.erase(line.begin(), line.begin() + md.name.length());
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), std::not1(std::ptr_fun<int,int>(isspace))));
        md.value = line;
        pmap->insert(md);
        is.clear();
        continue;
      }
      is.str(line);

      std::string moduleAndRegisterName;
      is >> moduleAndRegisterName;

      std::pair<std::string, std::string> moduleAndNamePair = splitStringAtLastDot(moduleAndRegisterName);
      module = moduleAndNamePair.first;
      name = moduleAndNamePair.second;
      if ( name.empty() ){
        std::ostringstream errorMessage;
        errorMessage << "Error in mapp file: Empty register name in line " << line_nr << "!";
        throw MapFileParserException(errorMessage.str(), LibMapException::EX_MAP_FILE_PARSE_ERROR);
      }

      is >> std::setbase(0) >> nElements >> std::setbase(0) >> address >> std::setbase(0) >> nBytes;
      if (!is){
        std::ostringstream os;
        os << line_nr;
        throw MapFileParserException("Error in map file: \"" + file_name + "\" in line (" + os.str() + ") \"" + line + "\"", LibMapException::EX_MAP_FILE_PARSE_ERROR);
      }
      // first, set default values for 'optional' fields
      bar = 0x0;
      width= 32;
      nFractionalBits = 0;
      signedFlag = true;
      is >> std::setbase(0) >> bar;
      if (is.fail()){
        failed = true;
      }
      if (!failed) {
        is >> std::setbase(0) >> width;
        if (is.fail()){
          failed = true;
        } else {
          if (width > 32) {
            std::ostringstream os;
            os << line_nr;
            throw MapFileParserException("Error in map file (register width too big): \"" + file_name + "\" in line (" + os.str() + ") \"" + line + "\"", LibMapException::EX_MAP_FILE_PARSE_ERROR);
          }
        }
      }
      if (!failed) {
        is >> std::setbase(0) >> nFractionalBits;
        if (is.fail()){
          failed = true;
        } else {
          if (nFractionalBits > 1023 || nFractionalBits < -1024) {
            std::ostringstream os;
            os << line_nr;
            throw MapFileParserException("Error in map file (too many fractional bits): \"" + file_name + "\" in line (" + os.str() + ") \"" + line + "\"", LibMapException::EX_MAP_FILE_PARSE_ERROR);
          }
        }
      }

      if (!failed) {
        is >> std::setbase(0) >> signedFlag;
        // no need to check if 'is' failed. Insert a check to set the failed flags if more fields are added
        //if (is.fail()){
        //  failed = true;
        //}
      }
      is.clear();
      lineNumber = line_nr;

      RegisterInfoMap::RegisterInfo registerInfo(name, nElements, address, nBytes, bar, width, nFractionalBits,
                                                 signedFlag, lineNumber, module);
      pmap->insert(registerInfo);
    }

    // search for 2D registers and add 2D entries
    std::vector<RegisterInfoMap::RegisterInfo> newInfos;
    for(auto &info: *pmap) {
      // check if 2D register, otherwise ignore
      if(info.name.substr(0, MULTIPLEXED_SEQUENCE_PREFIX.length()) != MULTIPLEXED_SEQUENCE_PREFIX) continue;
      // name of the 2D register is the name without the sequence prefix
      name = info.name.substr(MULTIPLEXED_SEQUENCE_PREFIX.length());
      // count number of channels and number of entries per channel
      size_t nChannels = 0;
      size_t nBytesPerEntry = 0;        // nb. of bytes per entry for all channels together
      auto cat = pmap->getRegisterCatalogue();
      while(cat.hasRegister( RegisterPath(info.module)/(SEQUENCE_PREFIX+name+"_"+std::to_string(nChannels)) )) {
        RegisterInfoMap::RegisterInfo subInfo;
        pmap->getRegisterInfo(RegisterPath(info.module)/(SEQUENCE_PREFIX+name+"_"+std::to_string(nChannels)), subInfo);
        nBytesPerEntry += subInfo.nBytes;
        nChannels++;
      }
      if(nChannels > 0) nElements = info.nBytes / nBytesPerEntry;
      // add it to the map
      RegisterInfoMap::RegisterInfo newEntry(name, nElements, info.address, info.nBytes, info.bar, info.width, info.nFractionalBits,
                                             info.signedFlag, info.lineNumber, info.module, nChannels, true);
      newInfos.push_back(newEntry);
    }
    // insert the new entries to the catalogue
    for(auto &entry: newInfos) {
      pmap->insert(entry);
    }

    return pmap;
  }

  std::pair<std::string, std::string> MapFileParser::splitStringAtLastDot( std::string moduleDotName){
    size_t lastDotPosition = moduleDotName.rfind('.');

    // some special case handlings to avoid string::split from throwing exceptions
    if (lastDotPosition == std::string::npos){
      // no dot found, the whole string is the second argument
      return std::make_pair( std::string(), moduleDotName );
    }

    if (lastDotPosition == 0){
      // the first character is a dot, everything from pos 1 is the second argument
      if (moduleDotName.size()==1){
        //it's just a dot, return  two empty strings
        return std::make_pair( std::string(), std::string() );
      }

      // split after the first character
      return std::make_pair( std::string(), moduleDotName.substr(1) );
    }

    if (lastDotPosition == (moduleDotName.size()-1) ){
      // the last character is a dot. The second argument is empty
      return std::make_pair( moduleDotName.substr(0, lastDotPosition),
          std::string() );
    }

    return std::make_pair( moduleDotName.substr(0, lastDotPosition),
        moduleDotName.substr(lastDotPosition+1) );
  }

}//namespace ChimeraTK

