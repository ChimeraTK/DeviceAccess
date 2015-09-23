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
    std::string _host;
		std::string _instance;
		std::list <std::string> _parameters;
public:
		DeviceBackendImpl() : _opened(false), _connected(true) {}
		DeviceBackendImpl(std::string host, std::string instance, std::list<std::string> parameters)
    : _opened(false), _connected(true) , _host(host), _instance(instance), _parameters(parameters){}
    virtual ~DeviceBackendImpl(){}
    /** Return whether a device has been opened or not.
     *  This is the only function implemented here to avoid code duplication.
     *  All other functions stay purely virtual.
     */
    virtual bool isOpen(){ return _opened; }
    virtual bool isConnected(){ return _connected; }
};

}//namespace mtca4u

#endif /*_MTCA4U_DEVICEBACKENDIMPL_H__*/
