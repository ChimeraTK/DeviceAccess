#ifndef LIBDEV_FAKE_H
#define	LIBDEV_FAKE_H

#include "devBase.h"
#include "devConfigBase.h"
#include <stdint.h>
#include <stdlib.h>

#define LIBDEV_BAR_NR          8
#define LIBDEV_BAR_MEM_SIZE    (1024*1024)


class devFake : public devBase
{
private:
    FILE*                 pcieMemory;
    std::string           pcieMemoryFileName;
public:
    devFake();
    ~devFake();

  
    virtual void openDev(const std::string &devName, int perm = O_RDWR, devConfigBase* pConfig = NULL);
    virtual void closeDev();
    
    virtual void readReg(uint32_t regOffset, int32_t* data, uint8_t bar);
    virtual void writeReg(uint32_t regOffset, int32_t data, uint8_t bar);
    
    virtual void readArea(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar);
    virtual void writeArea(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar);
    
    virtual void readDMA(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar);
    virtual void writeDMA(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar);
  
    virtual void readDeviceInfo(std::string* devInfo);
};

#endif	/* LIBDEV_STRUCT_H */

