/**
 *      @file           executiveThread.h
 *      @author         Adam Piotrowski <adam.piotrowski@desy.de>
 *      @version        1.0
 *      @brief          Provides support for thread used by executive environment       
 */
#ifndef EXECUTIVETHREAD_H
#define	EXECUTIVETHREAD_H

#include "threadBaseTSup.h"
#include "workerElemBase.h"
#include "condVariableTSup.h"
#include <string>
#include <list>

/**
 *      @brief  Provides support for thread used by executive environment 
 *      
 */
class executiveThread : public threadSupport::threadBase {
    private:
        std::list<std::string>          workerGroupsNames;              /**< list of workers groups names associated with thread*/
        std::list<workerElemBase*>      workerList;                     /**< list of workers associated with thread*/
        bool                            endThread;                      /**< true if thread has to be ended*/
        bool                            triggerWorker;                  /**< true if execution of workers associated with thread was triggered*/
        bool                            jobEnded;                       /**< true if workers ends job, false if worker still execute its work*/
        threadSupport::condVariableTSup condVarTriggerWorker;           /**< conditional variable used to trigger execution of workers associated with thread*/
        threadSupport::condVariableTSup condVarJobEnd;                  /**< conditional variable used to indicate that all warkers end jobs*/
    private:
        executiveThread(const executiveThread& orig);
    public:
        /**
         * 
         * @param workerGroupName
         * @return 
         */
        bool checkWorkerGroup(const std::string& workerGroupName);
        void addWorkerGroup(const std::string& workerGroupName);
        void addWorker(workerElemBase* pw);
        bool removeWorker(workerID id);
        int  getWorkerNumber();
        void triggerWorkers();
        void waitForJobEnd();
        executiveThread();
        bool checkIfIDInUse(workerID id);
    
        virtual ~executiveThread();
        virtual void killThread();
        virtual void run();
        friend std::ostream& operator<<(std::ostream& os, const executiveThread& et);
private:

};

#endif	/* EXECUTIVETHREAD_H */

