/* 
 * File:   workerDataAccess.cpp
 * Author: apiotro
 * 
 * Created on 27 styczeń 2012, 00:01
 */

#include "workerDataAccess.h"
#include "libdataAccess.h"
#include "singleton.h"

workerDataAccess::workerDataAccess(std::string logName) {
    dataAccess &da = dataAccessSingleton::Instance();
    pdataAccess = da.getDeviceObject(logName);
    data.init(pdataAccess->getDataSize());
}

workerDataAccess::workerDataAccess(const workerDataAccess& orig) {
}

workerDataAccess::~workerDataAccess() {
}

void workerDataAccess::run() {
    pdataAccess->readData(data);
}

std::string workerDataAccess::getWorkerGroupName() {
    std::string addr = pdataAccess->getAddress();
    size_t pos = addr.find_last_of(':');    
    return addr.substr(0, pos);
}

std::ostream& workerDataAccess::show(std::ostream& os) {
    os << pdataAccess->getAddress() << std::endl;
    return os;
}
