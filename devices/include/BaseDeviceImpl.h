#ifndef _MTCA4U_DEVBASEIMPL_H__
#define _MTCA4U_DEVBASEIMPL_H__

#include "BaseDevice.h"
#include <list>
namespace mtca4u{

/** BaseDevice implements the "opened" functionality which before was in BaseDevice.
 *  It is to be a base class for all the other implementations. Like this debBase
 *  becomes purely virtual, i.e. a real interface.
 */
class BaseDeviceImpl: public BaseDevice
{   
protected:
    bool        _opened;
    bool        _connected;
    std::string _host;
		std::string _interface;
		std::list <std::string> _parameters;
public:
    BaseDeviceImpl() : _opened(false), _connected(true) {}
    BaseDeviceImpl(std::string host, std::string interface, std::list<std::string> parameters)
    : _opened(false), _connected(true) , _host(host), _interface(interface), _parameters(parameters){}
    virtual ~BaseDeviceImpl(){}
    /** Return whether a device has been opened or not.
     *  This is the only function implemented here to avoid code duplication.
     *  All other functions stay purely virtual.
     */
    virtual bool isOpen(){ return _opened; }
    virtual bool isConnected(){ return _connected; }
};

}//namespace mtca4u

#endif /*_MTCA4U_LIBDEV_H__*/
