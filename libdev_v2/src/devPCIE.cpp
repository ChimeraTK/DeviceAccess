#include "devPCIE.h"
#include "exDevPCIE.h"
#include <iostream>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sstream>
#include <unistd.h>

devPCIE::devPCIE() 
  : dev_name(), dev_id(0)
{
    
}

devPCIE::~devPCIE()
{
    closeDev();
}

void devPCIE::openDev(const std::string &_devName, int perm, devConfigBase* /*pConfig*/)
{
    if (opened == true) {
        throw exDevPCIE("Device already has been opened", exDevPCIE::EX_DEVICE_OPENED);
    }   
    dev_name = _devName;
    dev_id = open(_devName.c_str(), perm);    
    if (dev_id < 0){
        throw exDevPCIE(std::string("Cannot open device: ") + dev_name + ": " + strerror_r(errno, errBuff, sizeof(errBuff)), exDevPCIE::EX_CANNOT_OPEN_DEVICE);
    }
    opened = true;
}

void devPCIE::closeDev()
{
    if (opened == true){
        close(dev_id);
    }
    opened = false;
}
    
void devPCIE::readReg(uint32_t regOffset, int32_t* data, uint8_t bar)
{
    device_rw	          l_RW;        
    if (opened == false) {
        throw exDevPCIE("Device closed", exDevPCIE::EX_DEVICE_CLOSED);
    }        
    l_RW.barx_rw   = bar;
    l_RW.mode_rw   = RW_D32;
    l_RW.offset_rw = regOffset;              
    if (read (dev_id, &l_RW, sizeof(device_rw)) != sizeof(device_rw)){ 
        throw exDevPCIE(std::string("Cannot read data from device: ") + dev_name + ": " + strerror_r(errno, errBuff, sizeof(errBuff)), exDevPCIE::EX_READ_ERROR);
    }    
    *data = l_RW.data_rw;            
}

void devPCIE::writeReg(uint32_t regOffset, int32_t data, uint8_t bar)
{    
    device_rw	          l_RW;        
    if (opened == false) {
        throw exDevPCIE("Device closed", exDevPCIE::EX_DEVICE_CLOSED);
    }    
    l_RW.barx_rw   = bar;
    l_RW.mode_rw   = RW_D32;
    l_RW.offset_rw = regOffset;
    l_RW.data_rw   = data;
    l_RW.rsrvd_rw  = 0;
    l_RW.size_rw   = 0;
    if (write (dev_id, &l_RW, sizeof(device_rw)) != sizeof(device_rw)){     
        throw exDevPCIE(std::string("Cannot write data from device: ") + dev_name + ": " + strerror_r(errno, errBuff, sizeof(errBuff)), exDevPCIE::EX_WRITE_ERROR);
    }       
}
    
void devPCIE::readArea(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar)
{       
    if (opened == false) {
        throw exDevPCIE("Device closed", exDevPCIE::EX_DEVICE_CLOSED);
    }     
    if (size % 4) {
        throw exDevPCIE("Wrong data size - must be dividable by 4", exDevPCIE::EX_READ_ERROR);
    } 
        
    for (uint32_t i = 0; i < size/4; i++){
        readReg(regOffset + i*4, data + i, bar);
    }     
}

void devPCIE::writeArea(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar)
{       
    if (opened == false) {
        throw exDevPCIE("Device closed", exDevPCIE::EX_DEVICE_CLOSED);
    }    
    if (size % 4) {
        throw exDevPCIE("Wrong data size - must be dividable by 4", exDevPCIE::EX_WRITE_ERROR);
    } 
    for (uint32_t i = 0; i < size/4; i++){
        writeReg(regOffset + i*4, *(data + i), bar);
    } 
}
    
void devPCIE::readDMA(uint32_t regOffset, int32_t* data, size_t size, uint8_t /*bar*/)
{   
    ssize_t	          ret;
    device_rw 	          l_RW;
    device_rw*            pl_RW;
            
    if (opened == false) {
        throw exDevPCIE("Device closed", exDevPCIE::EX_DEVICE_CLOSED);
    }         
    if (size < sizeof(device_rw)){
        pl_RW = &l_RW;
    } else {
        pl_RW = (device_rw*)data;
    }        
    
    pl_RW->data_rw   = 0;
    pl_RW->barx_rw   = 0;
    pl_RW->size_rw   = size;
    pl_RW->mode_rw   = RW_DMA;
    pl_RW->offset_rw = regOffset;    
    ret = read (dev_id, pl_RW, sizeof(device_rw));        
    if (ret != (ssize_t)size){           
        throw exDevPCIE(std::string("Cannot read data from device: ") + dev_name + ": " + strerror_r(errno, errBuff, sizeof(errBuff)), exDevPCIE::EX_WRITE_ERROR);
    }
    if (size < sizeof(device_rw)){
        memcpy(data, pl_RW, size);
    }
}

void devPCIE::writeDMA(uint32_t /*regOffset*/, int32_t* /*data*/, size_t /*size*/, uint8_t /*bar*/)
{
    throw exDevPCIE("Operation not supported yet", exDevPCIE::EX_DMA_WRITE_ERROR);
}

void devPCIE::readDeviceInfo(std::string* devInfo)
{    
    std::ostringstream    os;
    device_ioctrl_data	  ioctlData = {0, 0, 0, 0};
    if (ioctl(dev_id, LLRFDRV_PHYSICAL_SLOT, &ioctlData) < 0){
        throw exDevPCIE(std::string("Cannot read device info: ") + dev_name + ": " + strerror_r(errno, errBuff, sizeof(errBuff)), exDevPCIE::EX_INFO_READ_ERROR);        
    }
    os << "SLOT: " << ioctlData.data;
    if (ioctl(dev_id, LLRFDRV_DRIVER_VERSION, &ioctlData) < 0){
        throw exDevPCIE(std::string("Cannot read device info: ") + dev_name + ": " + strerror_r(errno, errBuff, sizeof(errBuff)), exDevPCIE::EX_INFO_READ_ERROR);                
    }
    os << " DRV VER: " << (float)(ioctlData.offset/10.0) + (float)ioctlData.data;
    *devInfo = os.str();
}
