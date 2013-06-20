/* 
 * File:   executiveBase.cc
 * Author: apiotro
 * 
 */

#include <iostream>
#include "workerElemBase.h"

workerElemBase::workerElemBase()
: id(0) {
}


workerElemBase::~workerElemBase() {
}

void workerElemBase::run()
{
    
}

void workerElemBase::setWorkerID(workerID _id)
{
    id = _id;
}

workerID workerElemBase::getWorkerID()
{
    return id;
}

std::ostream& workerElemBase::show(std::ostream& os)
{
    os << "workerID: " << id << std::endl;
    return os;
}
