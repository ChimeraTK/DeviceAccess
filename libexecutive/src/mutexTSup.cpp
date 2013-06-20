#include "mutexTSup.h"

namespace threadSupport {

void mutex::acquire()
{
    ::pthread_mutex_lock(&protector);
}

void mutex::release()
{
    ::pthread_mutex_unlock(&protector);
}

mutex::mutex()
{
    ::pthread_mutex_init(&protector, NULL);
}

mutex::~mutex()
{
    ::pthread_mutex_destroy(&protector);
}

bool mutex::trylock()
{
    return !pthread_mutex_trylock(&protector);
}

}