#include <iostream>

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

  MapFileParserException::MapFileParserException(const std::string &_exMessage, unsigned int _exID)
  : MapFileException(_exMessage, _exID)
  {

  }

  DMapFileParserException::DMapFileParserException(const std::string &_exMessage, unsigned int _exID)
  : DMapFileException(_exMessage, _exID)
  {

  }

}//namespace ChimeraTK
