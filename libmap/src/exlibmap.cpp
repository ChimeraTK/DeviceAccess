#include "exlibmap.h"
#include <iostream>

namespace mtca4u{

exLibMap::~exLibMap() throw()
{
    
}

exLibMap::exLibMap(const std::string &_exMessage, unsigned int _exID)
        : exBase(_exMessage, _exID)
{
    
}

std::ostream& operator<<(std::ostream &os, const exLibMap& e)
{    
    os << "(ID: " << e.exID << ")" << " " << e.exMessage;        
    return os;
}

exMapFile::exMapFile(const std::string &_exMessage, unsigned int _exID)
        : exLibMap(_exMessage, _exID)
{
    
}

exDmapFile::exDmapFile(const std::string &_exMessage, unsigned int _exID)
        : exLibMap(_exMessage, _exID)
{
    
}

exMapFileParser::exMapFileParser(const std::string &_exMessage, unsigned int _exID)
        : exMapFile(_exMessage, _exID)
{
    
}

exDmapFileParser::exDmapFileParser(const std::string &_exMessage, unsigned int _exID)
        : exDmapFile(_exMessage, _exID)
{
    
}

}//namespace mtca4u
