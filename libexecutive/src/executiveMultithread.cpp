/* 
 * File:   executive.cc
 * Author: apiotro
 * 
 */

#include "executiveMultithread.h"
#include "dbg_print.h"
#include <limits>
#include <ostream>

executiveMultithread::executiveMultithread(unsigned int _maxThreadNr)
: maxThreadNr(_maxThreadNr), currentWorkerID(0) {

}

executiveMultithread::~executiveMultithread() {
    destroy();
}

workerID executiveMultithread::registerWorker(workerElemBase* pwe) {
    bool inUse;
    std::vector<executiveThread*>::iterator iter;
    std::string workerGroupName = pwe->getWorkerGroupName();

    currentWorkerID++;
    
    do{
        inUse = false;
        for (iter = threadList.begin(); iter != threadList.end(); ++iter) {
            if ((*iter)->checkIfIDInUse(currentWorkerID)){
                currentWorkerID++;
                inUse = true;
                break;
            }            
        }
    } while(inUse);
        
    pwe->setWorkerID(currentWorkerID);
    for (iter = threadList.begin(); iter != threadList.end(); ++iter) {
        if ((*iter)->checkWorkerGroup(workerGroupName) == true) {
            (*iter)->addWorker(pwe);
            return currentWorkerID;
        }
    }
    if (threadList.size() < maxThreadNr) {
        executiveThread* et = new executiveThread;
        et->addWorkerGroup(workerGroupName);
        et->addWorker(pwe);
        et->startThread();
        threadList.push_back(et);
        return currentWorkerID;
    }
    std::vector<executiveThread*>::iterator minWorkerNrIter;
    int minWorkerNr = std::numeric_limits<int>::max();

    for (iter = threadList.begin(); iter != threadList.end(); ++iter) {
        if ((*iter)->getWorkerNumber() < minWorkerNr) {
            minWorkerNrIter = iter;
            minWorkerNr = (*iter)->getWorkerNumber();
        }
    }
    (*minWorkerNrIter)->addWorker(pwe);
    (*minWorkerNrIter)->addWorkerGroup(workerGroupName);
    return currentWorkerID;
}

void executiveMultithread::removeWorker(workerID id) {
    std::vector<executiveThread*>::iterator iter;
    for (iter = threadList.begin(); iter != threadList.end(); ++iter) {
        if ((*iter)->removeWorker(id) == true){
            break;
        }
    }
}

void executiveMultithread::run() {
    std::vector<executiveThread*>::iterator iter;
    for (iter = threadList.begin(); iter != threadList.end(); ++iter) {
        (*iter)->triggerWorkers();          
    }
    for (iter = threadList.begin(); iter != threadList.end(); ++iter) {
        (*iter)->waitForJobEnd();          
    }
    
}

std::ostream& operator<<(std::ostream& os, const executiveMultithread& obj) {
    std::vector<executiveThread*>::const_iterator iter;
    os << "Thread max: " << obj.maxThreadNr << std::endl;
    for (iter = obj.threadList.begin(); iter != obj.threadList.end(); ++iter) {
        os << *(*(iter)) << std::endl;
    }
    return os;
}

void executiveMultithread::destroy() {
    std::vector<executiveThread*>::const_iterator iter;
    for (iter = threadList.begin(); iter != threadList.end(); ++iter) {
        (*iter)->killThread();
        delete *iter;
    }
    threadList.clear();
}

