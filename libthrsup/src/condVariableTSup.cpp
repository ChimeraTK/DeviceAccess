/* 
 * File:   condVariableTSup.cpp
 * Author: apiotro
 * 
 * Created on 25 styczeń 2012, 15:41
 */


#include "condVariableTSup.h"

namespace threadSupport {

    condVariableTSup::condVariableTSup() {
        pthread_mutex_init(&countMutex, NULL);
        pthread_cond_init(&condVariable, NULL);
    }

    condVariableTSup::condVariableTSup(const condVariableTSup& orig) {
    }

    condVariableTSup::~condVariableTSup() {
        pthread_mutex_destroy(&countMutex);
        pthread_cond_destroy(&condVariable);
    }

    void condVariableTSup::waitForCondition() {
       pthread_cond_wait(&condVariable, &countMutex);        
    }

    void condVariableTSup::releaseConditionMutex() {
        pthread_mutex_unlock(&countMutex);
    }
    void condVariableTSup::aquireConditionMutex() {
        pthread_mutex_lock(&countMutex);
    }

    void condVariableTSup::signalCondition() {
        pthread_cond_broadcast(&condVariable);
    }
}

