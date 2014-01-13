#ifndef LIBDEV_STRUCT_H
#define	LIBDEV_STRUCT_H

#include "devBase.h"
#include <stdint.h>
#include <stdlib.h>


class devPCIE : public devBase
{
private:
    std::string _deviceName;
    int  _deviceID;
    
    unsigned long _ioctlPhysicalSlot;
    unsigned long _ioctlDriverVersion;
    
    /// A function pointer which calls the correct dma read function (via ioctl or via struct)
    void (devPCIE::* _readDMAFunction)(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar);

    void readDMAViaIoctl(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar);
    void readDMAViaStruct(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar);

    std::string createErrorStringWithErrnoText(std::string const & startText);
    void  determineDriverAndConfigureIoctl();
public:
    devPCIE();
    virtual ~devPCIE();
             
    virtual void openDev(const std::string &devName, int perm = O_RDWR, devConfigBase* pConfig = NULL);
    virtual void closeDev();
    
    virtual void readReg(uint32_t regOffset, int32_t* data, uint8_t bar);
    virtual void writeReg(uint32_t regOffset, int32_t data, uint8_t bar);
    
    virtual void readArea(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar);
    virtual void writeArea(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar);
    
    virtual void readDMA(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar);
    virtual void writeDMA(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar);

    virtual void readDeviceInfo(std::string* devInfo);

    static devBase * createInstance();
};

#endif	/* LIBDEV_STRUCT_H */

