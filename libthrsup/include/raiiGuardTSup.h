#ifndef _RAIIGUARD_H
#define	_RAIIGUARD_H
#include <pthread.h>

namespace threadSupport {

    template <typename T>
    class raiiGuard {
    private:
        T& protector;
        bool owner;
    private:
        raiiGuard(const raiiGuard& orig);

    public:
        raiiGuard(T& _protector, bool noAcq = false);
        virtual ~raiiGuard();
        void release();
        void acquire();
    };

    template <typename T>
    raiiGuard<T>::raiiGuard(T& _protector, bool noAcq) : protector(_protector), owner(false) {
        if (!noAcq) {
            protector.acquire();
            owner = true;
        }
    }

    template <typename T>
    raiiGuard<T>::raiiGuard(const raiiGuard& orig) : protector(orig.protector), owner(orig.owner) {

    }

    template <typename T>
    raiiGuard<T>::~raiiGuard() {
        if (owner) {
            protector.release();
            owner = false;
        }
    }

    template <typename T>
    void raiiGuard<T>::release() {
        protector.release();
        owner = false;
    }

    template <typename T>
    void raiiGuard<T>::acquire() {
        protector.acquire();
        owner = true;
    }

}

#endif	/* _RAIIGUARD_H */

