/* 
 * File:   condVariableTSup.h
 * Author: apiotro
 *
 */

#ifndef CONDVARIABLETSUP_H
#define	CONDVARIABLETSUP_H


#include <pthread.h>

namespace threadSupport {

    class condVariableTSup{
    private:
        pthread_cond_t  condVariable;
        pthread_mutex_t countMutex;
    private:
        condVariableTSup(const condVariableTSup& orig);
    public:
        condVariableTSup();        
        virtual ~condVariableTSup();
        void waitForCondition();
        void releaseConditionMutex();
        void aquireConditionMutex();
        void signalCondition();
    };

}

#endif	/* CONDVARIABLETSUP_H */

