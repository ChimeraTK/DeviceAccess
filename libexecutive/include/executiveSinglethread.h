/* 
 * File:   executiveSinglethread.h
 * Author: apiotro
 *
 * Created on 11 wrzesień 2012, 11:59
 */

#ifndef EXECUTIVESINGLETHREAD_H
#define	EXECUTIVESINGLETHREAD_H

#include "workerElemBase.h"
#include <list>

class executiveSinglethread {
private:
    std::list<workerElemBase*>      workerList;
    workerID                        currentWorkerID;
public:
    executiveSinglethread();
    virtual ~executiveSinglethread();
    
    void        destroy();
    workerID    registerWorker(workerElemBase* pwe);
    void        removeWorker(workerID id);
    void        run();
    friend std::ostream& operator<<(std::ostream& os, const executiveSinglethread& execute);
private:

};

#endif	/* EXECUTIVESINGLETHREAD_H */
