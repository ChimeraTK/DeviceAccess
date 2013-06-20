/* 
 * File:   exThread.cpp
 * Author: apiotro
 * 
 * Created on 25 styczeń 2012, 09:50
 */

#include "exThread.h"

exThread::exThread(const std::string &_exMessage, unsigned int _exID) 
 : exBase(_exMessage, _exID)
{
}

exThread::exThread(const exThread& orig) 
 : exBase(orig)
{
}

exThread::~exThread() throw() {
}


