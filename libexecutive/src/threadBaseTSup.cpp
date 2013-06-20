#include "threadBaseTSup.h"
#include "exThread.h"
#include "dbg_print.h"

namespace threadSupport {

    threadBase::threadBase(const std::string& _threadName) : threadId(0), threadName(_threadName) {
    }

    threadBase::threadBase(const threadBase& orig) {

    }

    threadBase::~threadBase() {
    }

    bool threadBase::isThreadStarted() {
        return (threadId != 0);
    };
    
    void threadBase::killThread()
    {
        pthread_join(threadId, NULL);
        threadId = 0;
    }

    void threadBase::startThread() {
        pthread_attr_t attr;
        int pthread_error;

        if ((pthread_error = pthread_attr_init(&attr)) != 0) {
            throw exThread("Cannot initialize pthread attribute structure", exThread::EX_CANNOT_CREATE_THREAD);
        }
        if ((pthread_error = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE)) != 0) {
            throw exThread("Cannot set detached state for threads", exThread::EX_CANNOT_CREATE_THREAD);
        }
        if ((pthread_error = pthread_create(&threadId, &attr, _threadfun, this)) != 0) {
            throw exThread("Cannot create process id thread", exThread::EX_CANNOT_CREATE_THREAD);
        }
        pthread_attr_destroy(&attr);
        threadStarted.acquire();
    }

    void* threadBase::_threadfun(void* threadData) {
        threadBase *data = reinterpret_cast<threadBase*> (threadData);
        data->threadStarted.release();
        data->run();
        pthread_exit(NULL);
    }

}
