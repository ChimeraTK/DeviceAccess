#include "libdev_serial.h"
#include <typeinfo>
#include <assert.h>

devAccess_Serial::devAccess_Serial()
{

}

devAccess_Serial::~devAccess_Serial()
{
    closeDev();
}

uint16_t devAccess_Serial::openDev(const std::string &dev_name, int perm, devConfig_Base* pconfig)
{
    devConfig_Serial *conf;
    conf = dynamic_cast<devConfig_Serial*>(pconfig);
    assert(conf != NULL);
    
    
    SET_STATUS(STATUS_CLOSED, "Device closed");
}

uint16_t devAccess_Serial::closeDev()
{
    SET_STATUS(STATUS_CLOSED, "Device closed");
}

uint16_t devAccess_Serial::writeData(int32_t* data, size_t &size)
{
    {SET_LAST_ERROR(NOT_SUPPORTED, "Operation not supported yet");}
}

uint16_t devAccess_Serial::readData(int32_t* data, size_t &size)
{
    {SET_LAST_ERROR(NOT_SUPPORTED, "Operation not supported yet");}
}
