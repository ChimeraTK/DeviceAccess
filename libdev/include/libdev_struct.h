#ifndef LIBDEV_STRUCT_H
#define	LIBDEV_STRUCT_H

#include "libdev_b.h"
#include <stdint.h>
#include <stdlib.h>


struct device_rw{
           u_int		offset_rw; /*offset in address*/
           u_int		data_rw;   /*data to set or returned read data */
           u_int		mode_rw;   /*mode of rw (RW_D8, RW_D16, RW_D32)*/
           u_int 		barx_rw;   /*BARx (0, 1, 2, 3)*/
           u_int 		size_rw;   /*transfer size in bytes*/
           u_int 		rsrvd_rw;  /*transfer size in bytes*/
};

class devAccess_Struct : public devAccess
{
private:
    std::string         dev_name;
    int                 dev_id;
    
public:
    devAccess_Struct();
    virtual ~devAccess_Struct();
             
    uint16_t openDev(const std::string &dev_name, int perm = O_RDWR, devConfig_Base* pconfig = NULL);
    uint16_t closeDev();
    
    uint16_t readReg(uint32_t reg_offset, int32_t* data, uint8_t bar);
    uint16_t writeReg(uint32_t reg_offset, int32_t data, uint8_t bar);
    
    uint16_t readArea(uint32_t reg_offset, int32_t* data, size_t &size, uint8_t bar);
    uint16_t writeArea(uint32_t reg_offset, int32_t* data, size_t &size, uint8_t bar);
    
    uint16_t readDMA(uint32_t reg_offset, int32_t* data, size_t &size, uint8_t bar);
    uint16_t writeDMA(uint32_t reg_offset, int32_t* data, size_t &size, uint8_t bar);
};

#endif	/* LIBDEV_STRUCT_H */

