#ifndef MUTEX_GUARD_H
#define	MUTEX_GUARD_H

#include "lockBaseTSup.h"
#include <pthread.h>

namespace threadSupport {

class mutex : lockBase{
private:
    pthread_mutex_t protector;                  
private:
    mutex(const mutex& orig);
public:
    friend class cond_var_t;
    mutex();
    virtual ~mutex();
    void   release();
    void   acquire();
    bool   trylock();
};

}
#endif	/* MUTEX_GUARD_H */

