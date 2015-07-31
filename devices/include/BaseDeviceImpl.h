#ifndef _MTCA4U_DEVBASEIMPL_H__
#define _MTCA4U_DEVBASEIMPL_H__

#include "BaseDevice.h"

namespace mtca4u{

/** BaseDevice implements the "opened" functionality which before was in BaseDevice.
 *  It is to be a base class for all the other implementations. Like this debBase
 *  becomes purely virtual, i.e. a real interface.
 */
class BaseDeviceImpl: public BaseDevice
{   
protected:
    bool         opened;
    bool         connected;
public:
    BaseDeviceImpl() : opened(false){}
    virtual ~BaseDeviceImpl(){}
             
    /** Return whether a device has been opened or not.
     *  This is the only function implemented here to avoid code duplication.
     *  All other functions stay purely virtual.
     */
    virtual bool isOpen(){ return opened; }
    virtual bool isConnected(){ return connected; }
};

}//namespace mtca4u

#endif /*_MTCA4U_LIBDEV_H__*/
