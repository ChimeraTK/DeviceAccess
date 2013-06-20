
#ifndef SEMAPHORE_T_H
#define	SEMAPHORE_T_H

#include "lockBaseTSup.h"
#include <semaphore.h>
#include <stdint.h>

namespace threadSupport {

class semaphore : public lockBase {
private:
    sem_t               semObj;    
private:
    semaphore(const semaphore& orig);
public:
    semaphore(uint32_t startVal = 0);
    virtual ~semaphore();
    void acquire();
    void release();
    bool trylock();
};

}

#endif	/* SEMAPHORE_T_H */

