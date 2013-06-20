#include "exDataProtocol.h"

exDataProtocol::exDataProtocol(const std::string &_exMessage, unsigned int _exID) 
: exBase(_exMessage, _exID)
{
}

exDataProtocol::~exDataProtocol() throw() {
}

