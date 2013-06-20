#include "semaphoreTSup.h"
#include <errno.h>

namespace threadSupport {

semaphore::semaphore(uint32_t start_val) {
    sem_init(&semObj, 0, start_val);
}

semaphore::semaphore(const semaphore& orig) {
}

semaphore::~semaphore() {
    sem_destroy(&semObj);
}

void semaphore::acquire() {
    while (1) {
        if (sem_wait(&semObj) == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                break;
            }
        } else {
            break;
        }
    }
}

void semaphore::release() {
    sem_post(&semObj);
}

bool semaphore::trylock() {
    if (sem_wait(&semObj) == -1) {
        return false;
    }
    return true;
}

}
