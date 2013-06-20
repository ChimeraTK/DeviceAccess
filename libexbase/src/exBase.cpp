#include "exBase.h"
#include <string>
#include <iostream>

exBase::exBase(const std::string &_exMessage, unsigned int _exID)
   : exMessage(_exMessage), exID(_exID)
{
}

exBase::~exBase() throw() 
{
}

const char* exBase::what() const throw()
{
    return exMessage.c_str();
}

unsigned int exBase::getID() const
{
    return exID;
}
