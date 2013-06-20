/* 
 * File:   executiveBase.h
 * Author: apiotro
 * 
 */

#ifndef WORKERELEMNASE_H
#define	WORKERELEMNASE_H

#include <string>
#include <stdint.h>

typedef uint32_t workerID;

class workerElemBase {
protected:
    workerID id;
public:
    void        setWorkerID(workerID _id);
    workerID    getWorkerID();
    workerElemBase();
    virtual ~workerElemBase();
    virtual void run() = 0;
    virtual std::string getWorkerGroupName() = 0;
    virtual std::ostream& show(std::ostream& os) = 0;
private:

};

#endif	/* WORKERELEMNASE_H */

