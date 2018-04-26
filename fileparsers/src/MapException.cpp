#include <iostream>
#include <sstream>

#include "MapException.h"

namespace ChimeraTK {

  LibMapException::~LibMapException() throw()
    {

    }

  LibMapException::LibMapException(const std::string &_exMessage, unsigned int _exID)
  : Exception(_exMessage, _exID)
  {

  }

  std::ostream& operator<<(std::ostream &os, const LibMapException& e)
  {
    os << "(ID: " << e.exID << ")" << " " << e.exMessage;        
    return os;
  }

  MapFileException::MapFileException(const std::string &_exMessage, unsigned int _exID)
  : LibMapException(_exMessage, _exID)
  {

  }

  DMapFileException::DMapFileException(const std::string &_exMessage, unsigned int _exID)
  : LibMapException(_exMessage, _exID)
  {

  }

  static std::string getParseExceptionMessage(const std::string &_fileName, unsigned int _lineNumber, const std::string &_line, const std::string &_errorMessage)
  {
    std::ostringstream oss;

    oss << "Error in map file: \""  << _fileName << "\" ";
    if (_errorMessage.length() > 0) {
        oss << "(" << _errorMessage << ") ";
    }
    oss << "in line (" << _lineNumber << ") \""
        << _line << "\"";

    return oss.str();
  }

  MapFileParserException::MapFileParserException(const std::string &_fileName, unsigned int _lineNumber, const std::string &_line, const std::string &_errorMessage)
      : MapFileException(getParseExceptionMessage(_fileName, _lineNumber, _line, _errorMessage), LibMapException::EX_MAP_FILE_PARSE_ERROR)
  {
  }

  DMapFileParserException::DMapFileParserException(const std::string &_exMessage, unsigned int _exID)
  : DMapFileException(_exMessage, _exID)
  {

  }

}//namespace ChimeraTK
