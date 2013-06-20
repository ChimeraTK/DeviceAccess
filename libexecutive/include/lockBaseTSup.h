#ifndef LOCKBASE_H
#define	LOCKBASE_H

namespace threadSupport {

class lockBase {
private:
    lockBase(const lockBase& orig); 
public:
    lockBase();    
    virtual ~lockBase();
    virtual void acquire() = 0;
    virtual void release() = 0;
    virtual bool trylock() = 0;
private:

};

}

#endif	/* LOCKBASE_H */

