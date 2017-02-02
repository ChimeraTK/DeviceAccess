#include <iostream>
#include <algorithm>
#include <sstream>
#include <fstream>

#include "DMapFileParser.h"
#include "Utilities.h"
#include "parserUtilities.h"
#include "MapException.h"

namespace utilities = mtca4u::parserUtilities;

namespace mtca4u {

  DeviceInfoMapPointer DMapFileParser::parse(const std::string &file_name) {
    std::ifstream file;
    std::string line;
    uint32_t line_nr = 0;

    std::string absPathToDMapFile = utilities::convertToAbsolutePath(file_name);

    file.open(absPathToDMapFile.c_str());
    if (!file) {        
      throw DMapFileParserException("Cannot open dmap file: \"" + absPathToDMapFile + "\"", LibMapException::EX_CANNOT_OPEN_DMAP_FILE);
    }

    DeviceInfoMapPointer dmap(new DeviceInfoMap(absPathToDMapFile));
    while (std::getline(file, line)) {
      line_nr++;
      line.erase(line.begin(), std::find_if(line.begin(), line.end(), std::not1(std::ptr_fun<int, int>(isspace))));
      if (!line.size()) {
        continue;
      }
      if (line[0] == '#') {
        continue;
      }
      if (line[0] == '@'){
	parseForLoadLib(file_name, line, line_nr, dmap);
	continue;
      }
      parseRegularLine(file_name, line, line_nr, dmap);
    }
    file.close();
    if (dmap->getSize() == 0) {
      throw DMapFileParserException("No data in in dmap file: \"" + file_name + "\"", LibMapException::EX_NO_DMAP_DATA);
    }
    return dmap;
  }

  void DMapFileParser::parseForLoadLib(std::string file_name, std::string line,
				       uint32_t line_nr, DeviceInfoMapPointer dmap){
    // we expect at leat two tokens: the key and the value
    std::istringstream s;
    std::string key, value;
    s.str(line);
    s >> key >> value;
    if (s && (key == "@LOAD_LIB")){
      dmap->addPluginLibrary(value);
    }else{
      raiseException(file_name, line, line_nr);
    }
  }

  void DMapFileParser::parseRegularLine(std::string file_name, std::string line,
					uint32_t line_nr, DeviceInfoMapPointer dmap){
    std::istringstream inStream;
    DeviceInfoMap::DeviceInfo deviceInfo;
	
    inStream.str(line);
    inStream >> deviceInfo.deviceName >> deviceInfo.uri >> deviceInfo.mapFileName;

    if (inStream) {
      // @todo FIXME: this can go up in the scope, never changes if file does not change
      std::string absPathToDMapFile = utilities::convertToAbsolutePath(file_name);

      // deviceInfo.mapFileName should contain the absolute path to the mapfile:
      std::string absPathToDmapDirectory = utilities::extractDirectory(absPathToDMapFile);
      std::string absPathToMapFile = utilities::concatenatePaths(absPathToDmapDirectory, deviceInfo.mapFileName);
      deviceInfo.mapFileName = absPathToMapFile;
      deviceInfo.dmapFileName = absPathToDMapFile;
      deviceInfo.dmapFileLineNumber = line_nr;
      dmap->insert(deviceInfo);
    } else {
      raiseException(file_name, line, line_nr);
    }
  }


  void DMapFileParser::raiseException(std::string file_name, std::string line, uint32_t line_nr){
    std::stringstream errorMessage;
    errorMessage << "Error in dmap file: \"" << file_name << "\" in line (" << line_nr
		 << ") \"" << line << "\"";
    throw DMapFileParserException(errorMessage.str(), LibMapException::EX_DMAP_FILE_PARSE_ERROR);
  }

  
}//namespace mtca4u
