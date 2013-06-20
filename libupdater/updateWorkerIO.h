/* 
 * File:   updateWorkerIO.h
 * Author: apiotro
 *
 * Created on 21 maj 2012, 13:23
 */

#ifndef UPDATEWORKERIO_H
#define	UPDATEWORKERIO_H

#include "include/libupdater.h"

class updateWorkerIO : public updateWorkerBase {
public:
    updateWorkerIO();
    virtual ~updateWorkerIO();
    virtual void run();
private:

};

#endif	/* UPDATEWORKERIO_H */

