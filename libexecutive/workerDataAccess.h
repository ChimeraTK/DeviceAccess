/* 
 * File:   workerDataAccess.h
 * Author: apiotro
 *
 */

#ifndef WORKERDATAACCESS_H
#define	WORKERDATAACCESS_H

#include "workerElemBase.h"
#include "dataProtocolElem.h"
#include "rawData.h"
#include <string>

class workerDataAccess : public workerElemBase {
public:
    dataProtocolElem    *pdataAccess;
    rawData             data;
public:
    workerDataAccess(std::string logName);
    workerDataAccess(const workerDataAccess& orig);
    virtual void run();
    virtual std::string getWorkerGroupName();
    virtual std::ostream& show(std::ostream& os);
    virtual ~workerDataAccess();
private:

};

#endif	/* WORKERDATAACCESS_H */

