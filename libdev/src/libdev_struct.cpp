#include "libdev_struct.h"
#include <iostream>
#include <string.h>

devAccess_Struct::devAccess_Struct() 
        : dev_id(0)
{
    
}

devAccess_Struct::~devAccess_Struct()
{
    closeDev();
}

uint16_t devAccess_Struct::openDev(const std::string &_dev_name, int perm, devConfig_Base* pconfig)
{    
    dev_name = _dev_name;
    dev_id = open(_dev_name.c_str(), perm);
    if (dev_id < 0){
        SET_STATUS(OPEN_FAILURE, std::string("Cannot open device \"") + dev_name + "\": " + strerror_r(errno, err_buff, sizeof(err_buff)));        
    }
    SET_STATUS(STATUS_OK, "OK");
}

uint16_t devAccess_Struct::closeDev()
{
    if (status == STATUS_OK){
        close(dev_id);        
    }
    SET_STATUS(STATUS_CLOSED, "Device closed");
}
    
uint16_t devAccess_Struct::readReg(uint32_t reg_offset, int32_t* data, uint8_t bar)
{
    device_rw	          l_RW;        
    if (status != STATUS_OK) SET_STATUS(STATUS_CLOSED, "Device closed");       
    l_RW.barx_rw   = bar;
    l_RW.mode_rw   = 2;
    l_RW.offset_rw = reg_offset;;                
    if (read (dev_id, &l_RW, sizeof(device_rw)) != sizeof(device_rw)){                 
        SET_LAST_ERROR(READ_FAILURE, std::string("Cannot read data from device: ") + strerror_r(errno, err_buff, sizeof(err_buff)));
    }    
    *data = l_RW.data_rw;    
    SET_LAST_ERROR(STATUS_OK, "OK");         
}

uint16_t devAccess_Struct::writeReg(uint32_t reg_offset, int32_t data, uint8_t bar)
{    
    device_rw	          l_RW;        
    if (status != STATUS_OK) SET_STATUS(STATUS_CLOSED, "Device closed");     
    l_RW.barx_rw   = bar;
    l_RW.mode_rw   = 2;
    l_RW.offset_rw = reg_offset;;
    l_RW.data_rw   = data;
    if (write (dev_id, &l_RW, sizeof(device_rw)) != sizeof(device_rw)){       
        SET_LAST_ERROR(WRITE_FAILURE, std::string("Cannot write data to device: ") + strerror_r(errno, err_buff, sizeof(err_buff)));
    }       
    SET_LAST_ERROR(STATUS_OK, "OK"); 
}
    
uint16_t devAccess_Struct::readArea(uint32_t reg_offset, int32_t* data, size_t &size, uint8_t bar)
{       
    if (status != STATUS_OK) SET_STATUS(STATUS_CLOSED, "Device closed");     
    if (size % 4) SET_LAST_ERROR(READ_FAILURE, "Wrong data size - must be dividable by 4");
        
    for (uint16_t i = 0; i < size/4; i++){
        if (readReg(reg_offset + i*4, data + i, bar) != STATUS_OK){            
            return last_error;
        }
    }
    return last_error;     
}

uint16_t devAccess_Struct::writeArea(uint32_t reg_offset, int32_t* data, size_t &size, uint8_t bar)
{       
    if (status != STATUS_OK) SET_STATUS(STATUS_CLOSED, "Device closed");     
    if (size % 4) SET_LAST_ERROR(READ_FAILURE, "Wrong data size - must be dividable by 4");
    for (uint16_t i = 0; i < size/4; i++){
        if (writeReg(reg_offset + i*4, *(data + i), bar) != STATUS_OK){
            return last_error;
        }
    }
    return last_error;  
}
    
uint16_t devAccess_Struct::readDMA(uint32_t reg_offset, int32_t* data, size_t &size, uint8_t bar)
{   
    ssize_t	          ret;
    device_rw 	          l_RW;
    device_rw*            pl_RW;
            
    if (status != STATUS_OK) SET_STATUS(STATUS_CLOSED, "Device closed");         
    if (size < sizeof(device_rw)){
        pl_RW = &l_RW;
    } else {
        pl_RW = (device_rw*)data;
    }        
    
    pl_RW->data_rw   = 0;
    pl_RW->barx_rw   = 0;
    pl_RW->size_rw   = size;
    pl_RW->mode_rw   = 3;
    pl_RW->offset_rw = reg_offset;                
    ret = read (dev_id, pl_RW, sizeof(device_rw));        
    if (ret != (ssize_t)size){      
        SET_LAST_ERROR(WRITE_FAILURE, std::string("Cannot read data from device: ") + strerror_r(errno, err_buff, sizeof(err_buff)));
    }
    if (size < sizeof(device_rw)){
        memcpy(data, pl_RW, size);
    }
        
    SET_LAST_ERROR(STATUS_OK, "OK");
}

uint16_t devAccess_Struct::writeDMA(uint32_t reg_offset, int32_t* data, size_t &size, uint8_t bar)
{
    SET_LAST_ERROR(NOT_SUPPORTED, "Operation not supported yet");
}

