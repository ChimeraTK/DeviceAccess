#ifndef LIBDEV_STRUCT_H
#define	LIBDEV_STRUCT_H

#include "devBase.h"
#include "llrfdrv_io.h"
#include <stdint.h>
#include <stdlib.h>


class devPCIE : public devBase
{
private:
    char errBuff[255];
    
    struct device_rw{
           u_int		offset_rw; /*offset in address*/
           u_int		data_rw;   /*data to set or returned read data */
           u_int		mode_rw;   /*mode of rw (RW_D8, RW_D16, RW_D32)*/
           u_int 		barx_rw;   /*BARx (0, 1, 2, 3)*/
           u_int 		size_rw;   /*transfer size in bytes*/
           u_int 		rsrvd_rw;  /*transfer size in bytes*/
    };
private:
    std::string         dev_name;
    int                 dev_id;
    
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
};

#endif	/* LIBDEV_STRUCT_H */

