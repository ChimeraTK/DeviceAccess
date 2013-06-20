/* 
 * File:   updateWorker.h
 * Author: apiotro
 *
 * Created on 21 maj 2012, 09:27
 */

#ifndef UPDATEWORKERBASE_H
#define	UPDATEWORKERBASE_H

class updateWorkerBase {
public:
    updateWorkerBase();
    virtual ~updateWorkerBase();
    virtual void run() = 0;
private:

};

#endif	/* UPDATEWORKERBASE_H */

