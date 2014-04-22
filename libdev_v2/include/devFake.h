#ifndef MTCA4U_LIBDEV_FAKE_H
#define	MTCA4U_LIBDEV_FAKE_H

#include "devBase.h"
#include "devConfigBase.h"
#include <stdint.h>
#include <stdlib.h>

#define MTCA4U_LIBDEV_BAR_NR          8
#define MTCA4U_LIBDEV_BAR_MEM_SIZE    (1024*1024)

namespace mtca4u{

class devFake : public devBase
{
private:
    FILE*                 pcieMemory;
    std::string           pcieMemoryFileName;
public:
    devFake();
    virtual ~devFake();

  
    virtual void openDev(const std::string &devName, int perm = O_RDWR, devConfigBase* pConfig = NULL);
    virtual void closeDev();
    
    virtual void readReg(uint32_t regOffset, int32_t* data, uint8_t bar);
    virtual void writeReg(uint32_t regOffset, int32_t data, uint8_t bar);
    
    virtual void readArea(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar);
    virtual void writeArea(uint32_t regOffset, int32_t const * data, size_t size, uint8_t bar);
    
    virtual void readDMA(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar);
    virtual void writeDMA(uint32_t regOffset, int32_t const * data, size_t size, uint8_t bar);
  
    virtual void readDeviceInfo(std::string* devInfo);

 private:
    /// A private copy constructor, cannot be called from outside. 
    /// As the default is not safe and I don't want to implement it right now, I just make it
    /// private. Make sure not to use it within the class before writing a proper implementation.
    devFake(devFake const &)  : pcieMemory(0), pcieMemoryFileName(){}

    /// A private assignment operator, cannot be called from outside. 
    /// As the default is not safe and I don't want to implement it right now, I just make it
    /// private. Make sure not to use it within the class before writing a proper implementation.
    devFake & operator=(devFake const &) {return *this;}
};

}//namespace mtca4u

#endif	/* MTCA4U_LIBDEV_STRUCT_H */

