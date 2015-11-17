#ifndef _MTCA4U_DEVICEBACKENDIMPL_H__
#define _MTCA4U_DEVICEBACKENDIMPL_H__

#include "DeviceBackend.h"
#include <list>
namespace mtca4u{

/** DeviceBackend implements the "opened" functionality which before was in DeviceBackend.
 *  It is to be a base class for all the other implementations. Like this debBase
 *  becomes purely virtual, i.e. a real interface.
 */
class DeviceBackendImpl: public DeviceBackend
{   
protected:
    bool        _opened;
    bool        _connected;

public:
    DeviceBackendImpl() : _opened(false), _connected(true) {}
    virtual ~DeviceBackendImpl(){}

    /** Return whether a device has been opened or not.
     *  This is the only function implemented here to avoid code duplication.
     *  All other functions stay purely virtual.
     */
    virtual bool isOpen(){ return _opened; }
    virtual bool isConnected(){ return _connected; }


    /** \deprecated {
     *  This function is deprecated. Use read() instead!
     *  @todo Add printed warning after release of version 0.2
     *  }
     */
    virtual void readDMA(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes) {
      read(bar, address, data,  sizeInBytes);
    }

    /** \deprecated {
     *  This function is deprecated. Use write() instead!
     *  @todo Add printed warning after release of version 0.2
     *  }
     */
    virtual void writeDMA(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes) {
      write(bar, address, data,  sizeInBytes);
    }

};

}//namespace mtca4u

#endif /*_MTCA4U_DEVICEBACKENDIMPL_H__*/
