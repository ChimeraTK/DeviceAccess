#ifndef __LIBDEV_MULTIFILE_H__
#define __LIBDEV_MULTIFILE_H__

#include "libdev_b.h"
#include <string>
#include <stdint.h>
  

class devAccess_Multifile : public devAccess
{
private:
    std::string         dev_name;
    int                 dev_id;
   
public:
    devAccess_Multifile();
    virtual ~devAccess_Multifile();
             
    uint16_t openDev(const std::string &dev_name, int perm = O_RDWR, devConfig_Base* pconfig = NULL);
    uint16_t closeDev();
    
    uint16_t readReg(uint32_t reg_offset, int32_t* data, uint8_t bar = 0);
    uint16_t writeReg(uint32_t reg_offset, int32_t data, uint8_t bar = 0);
    
    uint16_t readArea(uint32_t reg_offset, int32_t* data, size_t &size, uint8_t bar = 0);
    uint16_t writeArea(uint32_t reg_offset, int32_t* data, size_t &size, uint8_t bar = 0);
    
    uint16_t readDMA(uint32_t reg_offset, int32_t* data, size_t &size, uint8_t bar = 0);
    uint16_t writeDMA(uint32_t reg_offset, int32_t* data, size_t &size, uint8_t bar = 0);
};

#endif /*__LIBDEV_MULTIFILE_H__*/