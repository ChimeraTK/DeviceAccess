/**
 *      @file           executiveMultithread.h
 *      @author         Adam Piotrowski <adam.piotrowski@desy.de>
 *      @version        1.0
 *      @brief          Provides interface for multithread executive environment       
 */
#ifndef EXECUTIVE_H
#define	EXECUTIVE_H

#include "workerElemBase.h"
#include "executiveThread.h"
#include <vector>
#include <list>

/**
 *      @brief  Provides interface for multithread executive environment 
 *      
 *      Class allows to manage executive workers
 */
class executiveMultithread {
private:
    unsigned int                        maxThreadNr;            /**< maximum number of threads used by executive environment*/
    std::vector<executiveThread*>       threadList;             /**< list of worker threads */
    workerID                            currentWorkerID;        /**< last worker ID used by worker client*/
public:
    /**
     * @brief Class constructor
     * 
     * @param _maxThreadNr maximum number of threads used by executive environment
     * 
     */
    executiveMultithread(unsigned int _maxThreadNr = 4);
    virtual ~executiveMultithread();
    /**
     * @brief Registers new worker class in executive environment
     * 
     * @param pwe pointer to worker class
     * @return Returns worker ID associated to worker by executive environment
     */
    workerID    registerWorker(workerElemBase* pwe);
    /**
     * @brief Removes worker from executive environment 
     * 
     * @param ID of worker associated by registerWorker fuction
     */
    void        removeWorker(workerID id);
    /**
     * @brief Trigger one execution of all registered workers
     * 
     * Trigger one execution of all registered workers and waits until all workers finish execution
     */
    void        run();
    /**
     * @brief Stops all threads avaiable in executive environment and remove workers
     */
    void        destroy();
    friend std::ostream& operator<<(std::ostream& os, const executiveMultithread& execute);
private:

};

#endif	/* EXECUTIVE_H */

