/* 
 * File:   executiveSinglethread.cpp
 * Author: apiotro
 * 
 * Created on 11 wrzesień 2012, 11:59
 */

#include "executiveSinglethread.h"
#include <iostream>

executiveSinglethread::executiveSinglethread()
: currentWorkerID(0) {
}

executiveSinglethread::~executiveSinglethread() {
    destroy();
}

void executiveSinglethread::destroy() {
    std::list<workerElemBase*>::iterator iter;
    for (iter = workerList.begin(); iter != workerList.end(); ++iter) {
        delete *iter;
    }
    workerList.clear();
}

workerID executiveSinglethread::registerWorker(workerElemBase* pwe) {
    std::list<workerElemBase*>::iterator iter;
    bool idInUse = true;

    while (idInUse) {
        idInUse = false;
        currentWorkerID++;
        for (iter = workerList.begin(); iter != workerList.end(); ++iter) {
            if ((*iter)->getWorkerID() == currentWorkerID) {
                idInUse = true;
                break;
            }
        }
    }
    pwe->setWorkerID(currentWorkerID);
    workerList.push_back(pwe);
    return currentWorkerID;
}

void executiveSinglethread::removeWorker(workerID id) {
    std::list<workerElemBase*>::iterator iter;
    for (iter = workerList.begin(); iter != workerList.end(); ++iter) {
        if ((*iter)->getWorkerID() == id) {
            delete *iter;
            workerList.erase(iter);
            break;
        }
    }
}

void executiveSinglethread::run() {
    std::list<workerElemBase*>::iterator iter;
    for (iter = workerList.begin(); iter != workerList.end(); ++iter) {
        (*iter)->run();
    }
}

std::ostream& operator<<(std::ostream& os, const executiveSinglethread& execute) {
    std::list<workerElemBase*>::const_iterator iter;
    for (iter = execute.workerList.begin(); iter != execute.workerList.end(); ++iter) {
        (*iter)->show(os);
    }
    return os;
}


