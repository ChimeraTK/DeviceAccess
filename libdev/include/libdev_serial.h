/* 
 * File:   libdev_serial.h
 * Author: apiotro
 *
 */

#ifndef LIBDEV_SERIAL_H
#define	LIBDEV_SERIAL_H

#include "libdev_b.h"
#include <stdint.h>
#include <stdlib.h>


class devConfig_Serial : public devConfig_Base
{
public:
    int baudrate;
    int parity;
    int bitstop;
    int flowcontrol;
    
    virtual ~devConfig_Serial(){}
};

class devAccess_Serial : public devAccess
{
private:
    
    
public:
    devAccess_Serial();
    virtual ~devAccess_Serial();
             
    uint16_t openDev(const std::string &dev_name, int perm = O_RDWR, devConfig_Base* pconfig = NULL);
    uint16_t closeDev();
    
    uint16_t writeData(int32_t* data, size_t &size);
    uint16_t readData(int32_t* data, size_t &size);

};

#endif	/* LIBDEV_SERIAL_H */

