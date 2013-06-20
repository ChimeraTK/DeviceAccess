/* 
 * File:   updater.cc
 * Author: apiotro
 * 
 * Created on 21 maj 2012, 09:06
 */

#include "updateManager.h"
#include "dbg_print.h"

updateManager::workerElem::workerElem(const std::string &_workerName, updateWorkerBase* _pworker, uWorkerID _workerID, uint32_t _period)
: pworker(_pworker), workerID(_workerID), period(_period), enabled(true), executeCounter(0), currentCounter(0), workerName(_workerName) {
}

updateManager::disabledWorker::disabledWorker(const std::string& _workerName, uint32_t _period, uWorkerID _workerID)
: workerID(_workerID), period(_period), workerName(_workerName) {
}

updateManager::updateManager()
: mainClock(0), currentWorkerID(0) {

}

updateManager::~updateManager() {
    std::vector<workerElem>::iterator iter;
    for (iter = workersList.begin(); iter != workersList.end(); ++iter) {
        delete (*iter).pworker;
    }
}

uWorkerID updateManager::registerWorker(const std::string& workerName, updateWorkerBase* pworker, uint32_t period) {
    std::vector<workerElem>::iterator iter;
    bool inUse;

    currentWorkerID++;
    do {
        inUse = false;
        for (iter = workersList.begin(); iter != workersList.end(); ++iter) {
            if ((*iter).workerID == currentWorkerID) {
                currentWorkerID++;
                inUse = true;
                break;
            }
        }
    } while (inUse);

    workersList.push_back(workerElem(workerName, pworker, currentWorkerID, period));
    return currentWorkerID;
}

void updateManager::removeWorker(uWorkerID wID) {
    std::vector<workerElem>::iterator iter;
    for (iter = workersList.begin(); iter != workersList.end(); /*intentionally empty due to erasse*/) {
        if ((*iter).workerID == wID) {
            delete (*iter).pworker;
            workersList.erase(iter++);
        }
	else
	{
	  ++iter;
        }	
    }
}

bool updateManager::run(uint32_t newMainClock) {
    bool retValue = true;
    std::vector<workerElem>::iterator iter;
    if (newMainClock != mainClock) {
        mainClock = newMainClock;
        for (iter = workersList.begin(); iter != workersList.end(); ++iter) {
            if ((*iter).period < newMainClock) {
                (*iter).enabled = false;
                retValue = false;
                dbg_print("WORKER DISABLED: %s\n", (*iter).workerName.c_str());
                continue;
            }
            if ((*iter).period % newMainClock) {
                retValue = false;
                (*iter).enabled = false;
                dbg_print("WORKER DISABLED: %s (mainClock: %d, period: %d)\n", (*iter).workerName.c_str(), newMainClock, (*iter).period);
                continue;
            }
            (*iter).enabled = true;
            (*iter).executeCounter = (*iter).period / newMainClock;
            (*iter).currentCounter = 0;
        }
    }
    for (iter = workersList.begin(); iter != workersList.end(); ++iter) {
        if ((*iter).enabled) {
            (*iter).currentCounter++;
            if ((*iter).currentCounter == (*iter).executeCounter) {
                (*iter).currentCounter = 0;
                (*iter).pworker->run();
            }
        }
    }
    return retValue;
}

void updateManager::getDisabledWorkersList(std::vector<disabledWorker> &disList) {
    std::vector<workerElem>::iterator iter;
    disList.clear();
    for (iter = workersList.begin(); iter != workersList.end(); ++iter) {
        if (!(*iter).enabled) {
            disList.push_back(disabledWorker((*iter).workerName, (*iter).period, (*iter).workerID));
        }
    }
}

std::ostream& operator<<(std::ostream &os, const updateManager::workerElem& me) {
    os << "\tWORKER NAME  : " << me.workerName << std::endl;
    os << "\tWORKER ID    : " << me.workerID << std::endl;
    os << "\tPERIOD       : " << me.period << std::endl;
    os << "\tENABLED      : " << me.enabled << std::endl;
    os << "\tEXEC COUNTER : " << me.executeCounter << std::endl;
    os << "\tCURR COUNTER : " << me.currentCounter << std::endl;    
    return os;
}

std::ostream& operator<<(std::ostream &os, const updateManager::disabledWorker& me)
{
    os << "DISABLED WORKER:" << std::endl;
    os << "\tWORKER NAME  : "  << me.workerName << std::endl;    
    os << "\tPERIOD       : "  << me.period << std::endl;
    os << "\tWORKER ID    : " << me.workerID << std::endl;    
    return os;
}

std::ostream& operator<<(std::ostream &os, const updateManager& me) {
    std::vector<updateManager::workerElem>::const_iterator iter;
    os << "MAIN CLOCK   : " << me.mainClock << std::endl;
    for (iter = me.workersList.begin(); iter != me.workersList.end(); ++iter) {
        os << (*iter) << std::endl;
    }
    return os;
}
