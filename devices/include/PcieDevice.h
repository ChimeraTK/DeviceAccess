#ifndef MTCA4U_LIBDEV_STRUCT_H
#define	MTCA4U_LIBDEV_STRUCT_H

#include "BaseDeviceImpl.h"
#include <stdint.h>
#include <stdlib.h>
#include <boost/function.hpp>
#include "BaseDevice.h"

namespace mtca4u{

class PcieDevice : public BaseDeviceImpl
{
private:
    int  _deviceID;
    unsigned long _ioctlPhysicalSlot;
    unsigned long _ioctlDriverVersion;
    unsigned long _ioctlDMA;
    
    /// A function pointer which calls the correct dma read function (via ioctl or via struct)
    boost::function< void (uint32_t regOffset, int32_t* data, size_t size, uint8_t bar)>
      _readDMAFunction;

    /// A function pointer which call the right write function
    boost::function< void (uint32_t, int32_t const *, uint8_t) > _writeFunction;

    /// For the area we need something with a loop for the struct write.
    /// For the direct write this is the same as writeFunction.
    boost::function< void (uint32_t regOffset, int32_t const * data, uint8_t bar,
			   size_t sizeInBytes ) > _writeAreaFunction;

    boost::function<  void( uint32_t regOffset, int32_t * data, uint8_t bar) >
      _readFunction;

    boost::function<  void( uint32_t regOffset, int32_t * data, uint8_t bar,
			    size_t sizeInBytes ) > _readAreaFunction;

    void readDMAViaIoctl(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar);
    void readDMAViaStruct(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar);

    std::string createErrorStringWithErrnoText(std::string const & startText);
    void  determineDriverAndConfigureIoctl();
    void writeWithStruct(uint32_t regOffset, int32_t const * data, uint8_t bar);
    void writeAreaWithStruct(uint32_t regOffset, int32_t const * data,
			     uint8_t bar, size_t sizeInBytes );
    /** This function is the same for one or multiple words */
    void directWrite(uint32_t regOffset, int32_t const * data, uint8_t bar,
		     size_t sizeInBytes );

    void readWithStruct(uint32_t regOffset, int32_t* data, uint8_t bar);
    void readAreaWithStruct(uint32_t regOffset, int32_t* data, uint8_t bar, size_t size);
    /** This function is the same for one or multiple words */
    void directRead(uint32_t regOffset, int32_t* data, uint8_t bar, size_t sizeInBytes);

    /** constructor called through createInstance to create device object */
    PcieDevice(std::string host, std::string interface, std::list<std::string> parameters);

public:
    PcieDevice();

    virtual ~PcieDevice();

    virtual void open(const std::string &devName, int perm = O_RDWR, DeviceConfigBase* pConfig = NULL);
    virtual void open();
    virtual void close();
    
    virtual void readRaw(uint32_t regOffset, int32_t* data, uint8_t bar);
    virtual void writeRaw(uint32_t regOffset, int32_t data, uint8_t bar);
    
    virtual void readArea(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar);
    virtual void writeArea(uint32_t regOffset, int32_t const * data, size_t size, uint8_t bar);
    
    virtual void readDMA(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar);
    virtual void writeDMA(uint32_t regOffset, int32_t const * data, size_t size, uint8_t bar);

    virtual std::string readDeviceInfo();

    /*Host or parameters (at least for now) are just place holders as pcidevice does not use them*/
    static boost::shared_ptr<BaseDevice> createInstance(std::string host, std::string interface, std::list<std::string> parameters);
};

}//namespace mtca4u

#endif	/* MTCA4U_LIBDEV_STRUCT_H */

