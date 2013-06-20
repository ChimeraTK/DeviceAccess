#ifndef LIBDEV_FAKE_H
#define	LIBDEV_FAKE_H

#include "libdev_b.h"
#include <stdint.h>
#include <stdlib.h>

#define LIBDEV_BAR_NR          8
#define LIBDEV_BAR_MEM_SIZE    (1024*1024)


class devAccess_Fake : public devAccess
{
private:
    FILE*                 pcie_memory;
    
public:
    devAccess_Fake();
    ~devAccess_Fake();
             
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

