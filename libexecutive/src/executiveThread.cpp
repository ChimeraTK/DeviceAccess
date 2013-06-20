#include "executiveThread.h"
#include "threadBaseTSup.h"
#include <ostream>
#include <algorithm>

executiveThread::executiveThread()
: threadBase("executiveThread"), endThread(false), triggerWorker(false) {
}

executiveThread::executiveThread(const executiveThread& orig)
: threadBase(orig.threadName), endThread(false), triggerWorker(false) {
}

executiveThread::~executiveThread() {
    std::list<workerElemBase*>::const_iterator siter;
    for (siter = workerList.begin(); siter != workerList.end(); ++siter) {
        delete (*siter);
    }
}

bool executiveThread::checkWorkerGroup(const std::string& workerGroupName) {
    std::list<std::string>::iterator iter;
    if (workerGroupsNames.empty()) return true;
    iter = std::find(workerGroupsNames.begin(), workerGroupsNames.end(), workerGroupName);
    if (iter == workerGroupsNames.end()) {
        return false;
    }
    return true;
}

void executiveThread::addWorkerGroup(const std::string& workerGroupName) {
    condVarTriggerWorker.aquireConditionMutex();
    workerGroupsNames.push_back(workerGroupName);
    condVarTriggerWorker.releaseConditionMutex();
}

void executiveThread::addWorker(workerElemBase* pw) {
    condVarTriggerWorker.aquireConditionMutex();
    workerList.push_back(pw);
    condVarTriggerWorker.releaseConditionMutex();
}

bool executiveThread::checkIfIDInUse(workerID id) {
    condVarTriggerWorker.aquireConditionMutex();
    std::list<workerElemBase*>::iterator iter;
    for (iter = workerList.begin(); iter != workerList.end(); ++iter) {
        if ((*iter)->getWorkerID() == id){
            condVarTriggerWorker.releaseConditionMutex();
            return true;
        } 
    }
    condVarTriggerWorker.releaseConditionMutex();
    return false;
}

bool executiveThread::removeWorker(workerID id) {
    condVarTriggerWorker.aquireConditionMutex();
    std::list<workerElemBase*>::iterator iter;
    for (iter = workerList.begin(); iter != workerList.end(); ++iter) {
        if ((*iter)->getWorkerID() == id){
            delete *iter;
            workerList.erase(iter);
            condVarTriggerWorker.releaseConditionMutex();
            return true;
        } 
    }
    condVarTriggerWorker.releaseConditionMutex();
    return false;
}

int executiveThread::getWorkerNumber() {
    return workerList.size();
}

void executiveThread::killThread() {
    condVarTriggerWorker.aquireConditionMutex();
    endThread = true;
    triggerWorker   = true;
    jobEnded        = true;        
    condVarTriggerWorker.releaseConditionMutex();
    condVarTriggerWorker.signalCondition();
    threadBase::killThread();    
}

void executiveThread::waitForJobEnd()
{
    condVarJobEnd.aquireConditionMutex();
    while (1){
        if (jobEnded){
            condVarJobEnd.releaseConditionMutex();
            return;
        }
        condVarJobEnd.waitForCondition();
    }
}

void executiveThread::triggerWorkers() {
    condVarTriggerWorker.aquireConditionMutex();
    triggerWorker   = true;
    jobEnded        = false;
    condVarTriggerWorker.releaseConditionMutex();
    condVarTriggerWorker.signalCondition();
}

void executiveThread::run() {
    std::list<workerElemBase*>::iterator iter;
    while (1) {
        condVarTriggerWorker.aquireConditionMutex();
        while (1) {
            if (endThread) {
                condVarTriggerWorker.releaseConditionMutex();
                return;
            }
            if (triggerWorker) {                
                triggerWorker = false;                
                break;
            }
            condVarTriggerWorker.waitForCondition();
        }
        for (iter = workerList.begin(); iter != workerList.end(); ++iter) {
            (*iter)->run();
        }
        
        condVarTriggerWorker.releaseConditionMutex();
    
        condVarJobEnd.aquireConditionMutex();
        jobEnded = true;         
        condVarJobEnd.releaseConditionMutex();
        condVarJobEnd.signalCondition();
    }
}

std::ostream& operator<<(std::ostream& os, const executiveThread& et) {
    std::list<std::string>::const_iterator iter;
    os << "================================================" << std::endl;
    os << et.threadName << " [" << et.threadId << "]" << std::endl;
    os << "workerGroupsNames: ";
    for (iter = et.workerGroupsNames.begin(); iter != et.workerGroupsNames.end(); ++iter) {
        os << *iter << " ";
    }
    os << std::endl << "WORKERS:" << std::endl;

    std::list<workerElemBase*>::const_iterator siter;
    for (siter = et.workerList.begin(); siter != et.workerList.end(); ++siter) {
        (*siter)->show(os);
    }
    os << "================================================";
    return os;
}

