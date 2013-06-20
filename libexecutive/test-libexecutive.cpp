#include "libexecutive.h"
#include <iostream>
#include <string>

class workerIO : public workerElemBase {
private:
        std::string address;
public:
    workerIO(const std::string& _address);
    workerIO(const workerIO& orig);
    virtual ~workerIO();
    
    virtual void run();
    virtual std::string getWorkerGroupName();
    virtual std::ostream& show(std::ostream& os);
private:

};

#include "dbg_print.h"
#include "include/executiveSinglethread.h"
#include <iostream>

workerIO::workerIO(const std::string& _address) 
: address(_address)
{
}

workerIO::workerIO(const workerIO& orig) {
}

workerIO::~workerIO() {
}

void workerIO::run()
{    
    dbg_print("%s %d\n", "work in progress", id);
}

std::string workerIO::getWorkerGroupName()
{
    return address;
}        

std::ostream& workerIO::show(std::ostream& os)
{
    os << "------------------------------------------" << std::endl;
    workerElemBase::show(os);
    os << "ADDRESS: " << address << std::endl;
    os << "------------------------------------------" << std::endl;
    return os;
}

int main()
{   
    /*
    dataAccess &da = dataAccessSingleton::Instance();
    da.addProtocol(new dataProtocolPCIE());
    da.init("./demo_logicNameMapperFile.lmap", "demo_devMapFile.dmap");
    */
    executiveMultithread execute(4); 
   /* 
    execute.registerWorker(new workerDataAcceass("FIRMWARE_VER_ADC_1"));
    execute.registerWorker(new workerDataAccess("FIRMWARE_VER_ADC_2"));
    execute.registerWorker(new workerDataAccess("FIRMWARE_COMPILATION_1"));
*/
    execute.registerWorker(new workerIO("FIRMWARE_VER_ADC_1"));
    execute.registerWorker(new workerIO("FIRMWARE_VER_ADC_2"));
    execute.registerWorker(new workerIO("FIRMWARE_COMPILATION_1"));
    
    
    std::cout << execute << std::endl;
    
    
    std::cout << "+++++++++++++++++++++++++++++++++++++++" << std::endl;
    execute.run();    
    std::cout << "+++++++++++++++++++++++++++++++++++++++" << std::endl;
    execute.run();
    std::cout << "+++++++++++++++++++++++++++++++++++++++" << std::endl;
    execute.run();
    std::cout << "+++++++++++++++++++++++++++++++++++++++" << std::endl;
    
    execute.destroy();
    
    executiveSinglethread       execSingle;
    workerID                    wID;
    
    execSingle.registerWorker(new workerIO("S1"));
    wID = execSingle.registerWorker(new workerIO("S2"));
    execSingle.registerWorker(new workerIO("S3"));
    
    std::cout << execSingle << std::endl;
    
    std::cout << "+++++++++++++++++++++++++++++++++++++++" << std::endl;
    execSingle.run();    
    std::cout << "+++++++++++++++++++++++++++++++++++++++" << std::endl;
    execSingle.removeWorker(wID);
    std::cout << "+++++++++++++++++++++++++++++++++++++++" << std::endl;
    execSingle.run();    
    std::cout << "+++++++++++++++++++++++++++++++++++++++" << std::endl;   
    execSingle.destroy();
    return 0;
}
