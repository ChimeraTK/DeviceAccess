/* 
 * File:   updater.h
 * Author: apiotro
 *
 * Created on 21 maj 2012, 09:06
 */

#ifndef UPDATEMANAGER_H
#define	UPDATEMANAGER_H

#include "updateWorkerBase.h"
#include <vector>
#include <stdint.h>
#include <string>
#include <ostream>

typedef uint32_t uWorkerID;

class updateManager {
public:
    class workerElem {
    public:
        updateWorkerBase* pworker;
        uWorkerID workerID;
        uint32_t period;
        bool enabled;
        uint32_t executeCounter;
        uint32_t currentCounter;
        std::string workerName;
        workerElem(const std::string &_workerName, updateWorkerBase* _pworker, uWorkerID _workerID, uint32_t _period);
        friend std::ostream& operator<<(std::ostream &os, const updateManager::workerElem& me);
    };

private:
    uint32_t mainClock;
    std::vector<workerElem> workersList;
    uWorkerID currentWorkerID;

public:

    class disabledWorker {
    public:
        uWorkerID workerID;
        uint32_t period;
        std::string workerName;
        disabledWorker(const std::string &_workerName, uint32_t _period, uWorkerID _workerID);
        friend std::ostream& operator<<(std::ostream &os, const updateManager::disabledWorker& me);
    };

    updateManager();
    virtual ~updateManager();

    uWorkerID registerWorker(const std::string &workerName, updateWorkerBase* pworker, uint32_t period);
    void removeWorker(uWorkerID wID);
    void getDisabledWorkersList(std::vector<disabledWorker> &disList);
    bool run(uint32_t newMainClock);

    friend std::ostream& operator<<(std::ostream &os, const updateManager& me);
};

#endif	/* UPDATEMANAGER_H */