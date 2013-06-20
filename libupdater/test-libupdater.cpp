#include "./include/libupdater.h"
#include "updateWorkerIO.h"
#include <iostream>
#include <exception>
#include <string.h>

#define ITER_NR         10

int main()
{
    try {        
        updateManager   um;
        std::vector<updateManager::disabledWorker> dList;
        
        um.registerWorker("TEST1", new updateWorkerIO(), 3);                        
        for (int i = 0; i < ITER_NR; i++){
            if (!um.run(1)){
                um.getDisabledWorkersList(dList);
                std::vector<updateManager::disabledWorker>::iterator iter;
                for (iter = dList.begin(); iter != dList.end(); ++iter){
                    std::cout << (*iter) << std::endl;
                }
            }
            sleep(1);
        }        
        std::cout << um << std::endl;        
    } catch (std::exception &ex){
        std::cerr <<  ex.what() << std::endl;
    } catch (...){
        std::cerr << "Unknown exception" << std::endl;
    }
    return 0;
}
