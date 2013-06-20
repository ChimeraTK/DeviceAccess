#include "libdev_multifile.h"
#include <string.h>
#include <stdio.h> 
#include <iostream>

devAccess_Multifile::devAccess_Multifile() 
        : dev_id(0)
{
     
}

devAccess_Multifile::~devAccess_Multifile()
{
    closeDev();
}

uint16_t devAccess_Multifile::openDev(const std::string &_dev_name, int perm, devConfig_Base* pconfig)
{    
    dev_name = _dev_name;
    dev_id = open(_dev_name.c_str(), perm);
    if (dev_id < 0){
        SET_STATUS(OPEN_FAILURE, std::string("Cannot open device \"") + dev_name + "\": " + strerror_r(errno, err_buff, sizeof(err_buff)));                
    }
    SET_STATUS(STATUS_OK, "OK");
}

uint16_t devAccess_Multifile::closeDev()
{
    if (status == STATUS_OK){
        close(dev_id);        
    }
    SET_STATUS(STATUS_CLOSED, "Device closed");
}
    
uint16_t devAccess_Multifile::readReg(uint32_t reg_offset, int32_t* data, uint8_t bar)
{    
    if (status != STATUS_OK) SET_STATUS(STATUS_CLOSED, "Device closed");        
    if (pread(dev_id, data, sizeof(uint32_t), reg_offset) < 0){
        SET_LAST_ERROR(READ_FAILURE, std::string("Cannot read data from device: ") + strerror_r(errno, err_buff, sizeof(err_buff)));
    }
    SET_LAST_ERROR(STATUS_OK, "OK");   
}

uint16_t devAccess_Multifile::writeReg(uint32_t reg_offset, int32_t data, uint8_t bar)
{    
    if (status != STATUS_OK) SET_STATUS(STATUS_CLOSED, "Device closed");       
    if (pwrite(dev_id, &data, sizeof(uint32_t), reg_offset) < 0){
        SET_LAST_ERROR(WRITE_FAILURE, std::string("Cannot write data to device: ") + strerror_r(errno, err_buff, sizeof(err_buff)));
    }
    SET_LAST_ERROR(STATUS_OK, "OK"); 
}
    
uint16_t devAccess_Multifile::readArea(uint32_t reg_offset, int32_t* data, size_t &size, uint8_t bar)
{       
    if (status != STATUS_OK) SET_STATUS(STATUS_CLOSED, "Device closed");        
    if ((size = pread(dev_id, data, size, reg_offset)) < 0){
        SET_LAST_ERROR(READ_FAILURE, std::string("Cannot read data from device: ") + strerror_r(errno, err_buff, sizeof(err_buff)));
    }    
    SET_LAST_ERROR(STATUS_OK, "OK");   
}

uint16_t devAccess_Multifile::writeArea(uint32_t reg_offset, int32_t* data, size_t &size, uint8_t bar)
{
    if (status != STATUS_OK) SET_STATUS(STATUS_CLOSED, "Device closed");     
    if ((size = pwrite(dev_id, data, size, reg_offset)) < 0){
        SET_LAST_ERROR(WRITE_FAILURE, std::string("Cannot write data to device: ") + strerror_r(errno, err_buff, sizeof(err_buff)));
    }    
    SET_LAST_ERROR(STATUS_OK, "OK");     
}
    
uint16_t devAccess_Multifile::readDMA(uint32_t reg_offset, int32_t* data, size_t &size, uint8_t bar)
{
    return readArea(reg_offset, data, size);
}

uint16_t devAccess_Multifile::writeDMA(uint32_t reg_offset, int32_t* data, size_t &size, uint8_t bar)
{
    return writeArea(reg_offset, data, size);    
}