/* 
 * File:   singleton.h
 * Author: apiotro
 *
 */

#ifndef SINGLETON_H
#define	SINGLETON_H

#include <cstdlib>
#include <cstddef>
#include <pthread.h>
#include <stdexcept>
//===================================================================================================
template <
        class T,
        template <class> class CreationPolicy, 
        template <class> class LifetimePolicy, 
        template <class> class ThreadingModel,
        int V = 0
>
class SingletonHolder
{
    public:
        static T& Instance();
    private:
        SingletonHolder();
        static void DestroySingleton();
        static T                *pInstance;
        static bool             destroyed;
            
};

template <
        class T, 
        template <class> class CreationPolicy, 
        template <class> class LifetimePolicy, 
        template <class> class ThreadingModel,
        int V
>
T* SingletonHolder<T, CreationPolicy, LifetimePolicy, ThreadingModel, V>::pInstance = NULL;

template <
        class T, 
        template <class> class CreationPolicy, 
        template <class> class LifetimePolicy, 
        template <class> class ThreadingModel,
        int V
>
bool SingletonHolder<T, CreationPolicy, LifetimePolicy, ThreadingModel, V>::destroyed = false;

template <
        class T, 
        template <class> class CreationPolicy, 
        template <class> class LifetimePolicy, 
        template <class> class ThreadingModel,
        int V
>
void SingletonHolder<T, CreationPolicy, LifetimePolicy, ThreadingModel, V>::DestroySingleton()
{
    CreationPolicy<T>::Destroy(pInstance);
    pInstance = 0;
    destroyed = true;
}

template <
        class T, 
        template <class> class CreationPolicy, 
        template <class> class LifetimePolicy, 
        template <class> class ThreadingModel,
        int V
>
T& SingletonHolder<T, CreationPolicy, LifetimePolicy, ThreadingModel, V>::Instance()
{
    if (pInstance == NULL){
        typename ThreadingModel<T>::Lock guard __attribute__ ((unused));
        if (pInstance == NULL){
            if (destroyed){
                LifetimePolicy<T>::OnDeadReference();
                destroyed = false;
            }            
            pInstance = CreationPolicy<T>::Create();
            LifetimePolicy<T>::ScheduleDestruction(&DestroySingleton);
        }
        
    }
    return *pInstance;
}
//===================================================================================================
template <class T>
class CreateByNew
{
    public:
        static T*    Create() {return new T;}
        static void  Destroy(T* pObj) {delete pObj;}
};
//===================================================================================================
template <class T>
class SingleThread
{
    public:
        class Lock
        {
            
        };
};
//===================================================================================================
template <class T>
class ClassLevelLockable
{
    public:
        class Lock
        {
            private:
                static pthread_mutex_t mutex;
            public:
                Lock() {pthread_mutex_lock(&mutex);}
                Lock(T& obj) {}
                ~Lock() {pthread_mutex_unlock(&mutex);}
        };
};

template <class T>
pthread_mutex_t ClassLevelLockable<T>::Lock::mutex = PTHREAD_MUTEX_INITIALIZER;
//===================================================================================================
template <class T>
class LifetimeStandard
{
    public:
        static void ScheduleDestruction(void (*pFun)(void)) {atexit(pFun);}
        static void OnDeadReference() {throw std::runtime_error("Internal error: dead reference in singleton class");}
        
};
//===================================================================================================

#endif	/* SINGLETON_H */

