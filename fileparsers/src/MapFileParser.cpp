#include "MapFileParser.h"
#include "MapException.h"
#include <iostream>
#include <algorithm>
#include <sstream>

namespace mtca4u{

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
	RegisterInfoMap::RegisterInfo registerInfo;

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
		registerInfo.module = moduleAndNamePair.first;
		registerInfo.name = moduleAndNamePair.second;
		if ( registerInfo.name.empty() ){
			std::ostringstream errorMessage;
			errorMessage << "Error in mapp file: Empty register name in line " << line_nr << "!";
			throw MapFileParserException(errorMessage.str(), LibMapException::EX_MAP_FILE_PARSE_ERROR);
		}

		is >> std::setbase(0) >> registerInfo.nElements >> std::setbase(0) >> registerInfo.address >> std::setbase(0) >> registerInfo.nBytes;
		if (!is){
			std::ostringstream os;
			os << line_nr;
			throw MapFileParserException("Error in map file: \"" + file_name + "\" in line (" + os.str() + ") \"" + line + "\"", LibMapException::EX_MAP_FILE_PARSE_ERROR);
		}
		// first, set default values for 'optional' fields
		registerInfo.bar = 0x0;
		registerInfo.width= 32;
		registerInfo.nFractionalBits = 0;
		registerInfo.signedFlag = true;
		is >> std::setbase(0) >> registerInfo.bar;
		if (is.fail()){
			failed = true;
		}
		if (!failed) {
			is >> std::setbase(0) >> registerInfo.width;
			if (is.fail()){
				failed = true;
			} else {
				if (registerInfo.width > 32) {
					std::ostringstream os;
					os << line_nr;
					throw MapFileParserException("Error in map file (register width too big): \"" + file_name + "\" in line (" + os.str() + ") \"" + line + "\"", LibMapException::EX_MAP_FILE_PARSE_ERROR);
				}
			}
		}
		if (!failed) {
			is >> std::setbase(0) >> registerInfo.nFractionalBits;
			if (is.fail()){
				failed = true;
			} else {
				if (registerInfo.nFractionalBits > 1023 || registerInfo.nFractionalBits < -1024) {
					std::ostringstream os;
					os << line_nr;
					throw MapFileParserException("Error in map file (too many fractional bits): \"" + file_name + "\" in line (" + os.str() + ") \"" + line + "\"", LibMapException::EX_MAP_FILE_PARSE_ERROR);
				}
			}
		}

		if (!failed) {
			is >> std::setbase(0) >> registerInfo.signedFlag;
			// no need to check if 'is' failed. Insert a check to set the failed flags if more fields are added
			//if (is.fail()){
			//  failed = true;
			//}
		}
		is.clear();
		registerInfo.lineNumber = line_nr;
		pmap->insert(registerInfo);
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

}//namespace mtca4u

