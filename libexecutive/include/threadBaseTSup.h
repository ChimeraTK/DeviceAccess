#ifndef THREADBASE_T_H
#define	THREADBASE_T_H

#include <pthread.h>
#include <string>
#include "semaphoreTSup.h"

namespace threadSupport {

    class threadBase {
    public:
        threadBase(const std::string& _threadName);
        virtual ~threadBase();
        virtual void killThread() = 0;
        virtual void startThread();
        virtual bool isThreadStarted();

    protected:
        pthread_t       threadId;
        std::string     threadName;

    private:
        threadBase(const threadBase& orig);
        semaphore threadStarted;
        static void* _threadfun(void* threadData);
        virtual void run() = 0;
    };

}
#endif	/* THREADBASE_T_H */

