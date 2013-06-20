
#ifndef EXTHREAD_H
#define	EXTHREAD_H

#include "exBase.h"

class exThread : public exBase {
public:
    enum {EX_CANNOT_CREATE_THREAD};
public:
    exThread(const std::string &_exMessage, unsigned int _exID);
    exThread(const exThread& orig);
    virtual ~exThread() throw();
private:

};

#endif	/* EXTHREAD_H */

