#include "Exception.h"

#include <string>
#include <iostream>

namespace mtca4u{

ExcBase::ExcBase(const std::string &_exMessage, unsigned int _exID)
   : exMessage(_exMessage), exID(_exID)
{
}

ExcBase::~ExcBase() throw() 
{
}

const char* ExcBase::what() const throw()
{
    return exMessage.c_str();
}

unsigned int ExcBase::getID() const
{
    return exID;
}

}//namespace mtca4u
