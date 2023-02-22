// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DMapFileParser.h"

#include "parserUtilities.h"
// #include "Utilities.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

namespace utilities = ChimeraTK::parserUtilities;

namespace ChimeraTK {

  DeviceInfoMapPointer DMapFileParser::parse(const std::string& file_name) {
    std::ifstream file;
    std::string line;
    uint32_t line_nr = 0;

    std::string absPathToDMapFile = utilities::convertToAbsolutePath(file_name);

    file.open(absPathToDMapFile.c_str());
    if(!file) {
      throw ChimeraTK::logic_error("Cannot open dmap file: \"" + absPathToDMapFile + "\"");
    }

    DeviceInfoMapPointer dmap(new DeviceInfoMap(absPathToDMapFile));
    while(std::getline(file, line)) {
      line_nr++;
      line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](int c) { return !isspace(c); }));
      if(line.empty()) {
        continue;
      }
      if(line[0] == '#') {
        continue;
      }
      if(line[0] == '@') {
        parseForLoadLib(file_name, line, line_nr, dmap);
        continue;
      }
      parseRegularLine(file_name, line, line_nr, dmap);
    }
    file.close();
    if(dmap->getSize() == 0) {
      // need to throw special exception for the DMapFilesParser - this is
      // actually a ChimeraTK::logic_error exception!
      throw ChimeraTK::detail::EmptyDMapFileException("No data in in dmap file: \"" + file_name + "\"");
    }
    return dmap;
  }

  void DMapFileParser::parseForLoadLib(
      const std::string& file_name, const std::string& line, uint32_t line_nr, const DeviceInfoMapPointer& dmap) {
    // we expect at leat two tokens: the key and the value
    std::istringstream s;
    std::string key, value;
    s.str(line);
    s >> key >> value;
    if(s && (key == "@LOAD_LIB")) {
      dmap->addPluginLibrary(absPathOfDMapContent(value, file_name));
    }
    else {
      raiseException(file_name, line, line_nr);
    }
  }

  void DMapFileParser::parseRegularLine(
      const std::string& file_name, const std::string& line, uint32_t line_nr, const DeviceInfoMapPointer& dmap) {
    std::istringstream inStream;
    DeviceInfoMap::DeviceInfo deviceInfo;

    inStream.str(line);
    inStream >> deviceInfo.deviceName >> deviceInfo.uri >> deviceInfo.mapFileName;

    if(inStream) {
      std::string absPathToDMapFile = utilities::convertToAbsolutePath(file_name);
      std::string absPathToMapFile = absPathOfDMapContent(deviceInfo.mapFileName, file_name);
      deviceInfo.mapFileName = absPathToMapFile;
      deviceInfo.dmapFileName = absPathToDMapFile;
      deviceInfo.dmapFileLineNumber = line_nr;
      dmap->insert(deviceInfo);
    }
    else {
      std::istringstream inStream2;
      inStream2.str(line);
      inStream2 >> deviceInfo.deviceName >> deviceInfo.uri;

      if(inStream2) {
        std::string absPathToDMapFile = utilities::convertToAbsolutePath(file_name);
        deviceInfo.mapFileName = "";
        deviceInfo.dmapFileName = absPathToDMapFile;
        deviceInfo.dmapFileLineNumber = line_nr;
        dmap->insert(deviceInfo);
      }
      else {
        raiseException(file_name, line, line_nr);
      }
    }
  }

  std::string DMapFileParser::absPathOfDMapContent(const std::string& dmapContent, const std::string& dmapFileName) {
    // first determine the absolute path to the dmap file
    std::string absPathToDMapFile = utilities::convertToAbsolutePath(dmapFileName);
    // then extract the directory
    std::string absPathToDmapDirectory = utilities::extractDirectory(absPathToDMapFile);
    // now concatenate the dmap diretory to the entry in the dmap file (if the
    // latter is not absolute)
    return utilities::concatenatePaths(absPathToDmapDirectory, dmapContent);
  }

  void DMapFileParser::raiseException(const std::string& file_name, const std::string& line, uint32_t line_nr) {
    std::stringstream errorMessage;
    errorMessage << "Error in dmap file: \"" << file_name << "\" in line (" << line_nr << ") \"" << line << "\"";
    throw ChimeraTK::logic_error(errorMessage.str());
  }

} // namespace ChimeraTK
