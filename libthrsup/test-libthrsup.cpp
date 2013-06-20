#include "libthrsup.h"
#include <iostream>
#include <string>
#include "dbg_print.h"
#include "include/threadBaseTSup.h"
#include <iostream>

class exampleThread : public threadSupport::threadBase {
    public:
        exampleThread();
        void killThread();
    private:
        void run();
};

exampleThread::exampleThread()
: threadSupport::threadBase("ET") 
{
    
}

void exampleThread::run() {
    std::cout << "THREAD STARTED" << std::endl;
}

void exampleThread::killThread() {
    threadSupport::threadBase::killThread();
}

int main()
{   
    exampleThread et;
    et.startThread();
    et.killThread();
    return 0;
}
