#include "Exception.h"

#include <string>
#include <iostream>

namespace ChimeraTK{

Exception::Exception(const std::string &_exMessage, unsigned int _exID)
: exMessage(_exMessage), exID(_exID)
{
}

Exception::~Exception() throw() 
{

}

const char* Exception::what() const throw()
{
	return exMessage.c_str();
}

unsigned int Exception::getID() const
{
	return exID;
}

}//namespace ChimeraTK
