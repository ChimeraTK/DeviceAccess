#include "devPCIE.h"
#include "exDevPCIE.h"
#include <iostream>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sstream>
#include <unistd.h>

// the io constants and struct for the driver
// FIXME: they should come from the installed driver
#include <pciedev_io.h>
#include <llrfdrv_io_compat.h>

namespace mtca4u{

devPCIE::devPCIE() 
  : _deviceName(), _deviceID(0), _ioctlPhysicalSlot(0), _ioctlDriverVersion(0),
    _readDMAFunction(NULL)
{
}

devPCIE::~devPCIE()
{
    closeDev();
}

void devPCIE::openDev(const std::string &devName, int perm, devConfigBase* /*pConfig*/)
{
    if (opened == true) {
        throw exDevPCIE("Device already has been opened", exDevPCIE::EX_DEVICE_OPENED);
    }   
    _deviceName = devName;
    _deviceID = open(devName.c_str(), perm);    
    if (_deviceID < 0){
      throw exDevPCIE(createErrorStringWithErrnoText("Cannot open device: "),
		      exDevPCIE::EX_CANNOT_OPEN_DEVICE);
    }

    determineDriverAndConfigureIoctl();

    opened = true;
}

void devPCIE::determineDriverAndConfigureIoctl(){
  // determine the driver by trying the physical slot ioctl
    device_ioctrl_data	ioctlData = {0, 0, 0, 0};

    if (ioctl(_deviceID,  PCIEDEV_PHYSICAL_SLOT, &ioctlData) >= 0){
      // it's the pciedev driver
      _ioctlPhysicalSlot =  PCIEDEV_PHYSICAL_SLOT;
      _ioctlDriverVersion =  PCIEDEV_DRIVER_VERSION;
      _readDMAFunction = &devPCIE::readDMAViaIoctl;
      
      return;
    }
   
    if (ioctl(_deviceID,  LLRFDRV_PHYSICAL_SLOT, &ioctlData) >= 0){
      // it's the llrf driver
      _ioctlPhysicalSlot =  LLRFDRV_PHYSICAL_SLOT;
      _ioctlDriverVersion =  LLRFDRV_DRIVER_VERSION;
      _readDMAFunction = &devPCIE::readDMAViaStruct;
      
      return;
    }

    // No working driver. Close the device and throw an exception.
    std::cerr <<"Unsupported driver. " << createErrorStringWithErrnoText("Error is ") <<std::endl;;
    close(_deviceID);
    throw exDevPCIE("Unsupported driver in device" + _deviceName, exDevPCIE::EX_UNSUPPORTED_DRIVER);
}

void devPCIE::closeDev()
{
    if (opened == true){
        close(_deviceID);
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
    l_RW.size_rw   = 0; // does not overwrite the struct but writes one word back to data
    l_RW.data_rw   = -1;
    l_RW.rsrvd_rw = 0;
            
    if (read (_deviceID, &l_RW, sizeof(device_rw)) != sizeof(device_rw)){ 
        throw exDevPCIE(createErrorStringWithErrnoText("Cannot read data from device: "),
			exDevPCIE::EX_READ_ERROR);
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

    if (write (_deviceID, &l_RW, sizeof(device_rw)) != sizeof(device_rw)){     
        throw exDevPCIE(createErrorStringWithErrnoText("Cannot write data from device: "),
			exDevPCIE::EX_WRITE_ERROR);
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

void devPCIE::readDMA(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar)
{
    if (opened == false) {
        throw exDevPCIE("Device closed", exDevPCIE::EX_DEVICE_CLOSED);
    }    

    (this->*_readDMAFunction)(regOffset, data, size, bar);
}


void devPCIE::readDMAViaStruct(uint32_t regOffset, int32_t* data, size_t size, uint8_t /*bar*/)
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
    pl_RW->rsrvd_rw = 0;

    ret = read (_deviceID, pl_RW, sizeof(device_rw));        
    if (ret != (ssize_t)size){           
        throw exDevPCIE(createErrorStringWithErrnoText("Cannot read data from device: "),
			exDevPCIE::EX_DMA_READ_ERROR);
    }
    if (size < sizeof(device_rw)){
        memcpy(data, pl_RW, size);
    }
}

void devPCIE::readDMAViaIoctl(uint32_t regOffset, int32_t* data, size_t size, uint8_t /*bar*/)
{
    if (opened == false) {
      throw exDevPCIE("Device closed", exDevPCIE::EX_DEVICE_CLOSED);
    }    
    
    //safety check: the requested dma size (size of the data buffer) has to be at least
    // the size of the dma struct, because the latter has to be copied into the data buffer.
    if (size <  sizeof (device_ioctrl_dma)) {
      throw  exDevPCIE("Reqested dma size is too small", exDevPCIE::EX_DMA_READ_ERROR);
    }

    // prepare the struct
    device_ioctrl_dma   DMA_RW;
    DMA_RW.dma_cmd     = 0; //FIXME: Why is it 0? => read driver code
    DMA_RW.dma_pattern = 0; //FIXME: Why is it 0? => read driver code
    DMA_RW.dma_size    = size;
    DMA_RW.dma_offset  = regOffset;
    DMA_RW.dma_reserved1 = 0; //FIXME: is this a correct value?
    DMA_RW.dma_reserved2 = 0; //FIXME: is this a correct value?
   
    // the ioctrl_dma struct is copied to the beginning of the data buffer,
    // so the information about size and offset are passed to the driver.
    memcpy((void*)data, &DMA_RW, sizeof (device_ioctrl_dma));
    int ret = ioctl (_deviceID, PCIEDEV_READ_DMA, (void*)data);
    if ( ret ){           
      throw exDevPCIE( createErrorStringWithErrnoText("Cannot read data from device "),
		       exDevPCIE::EX_DMA_READ_ERROR);
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
    if (ioctl(_deviceID,  _ioctlPhysicalSlot, &ioctlData) < 0){
      throw exDevPCIE(createErrorStringWithErrnoText("Cannot read device info: "),
		       exDevPCIE::EX_INFO_READ_ERROR);        
    }
    os << "SLOT: " << ioctlData.data;
    if (ioctl(_deviceID, _ioctlDriverVersion, &ioctlData) < 0){
        throw exDevPCIE(createErrorStringWithErrnoText("Cannot read device info: "),
			exDevPCIE::EX_INFO_READ_ERROR);                
    }
    os << " DRV VER: " << (float)(ioctlData.offset/10.0) + (float)ioctlData.data;
    *devInfo = os.str();
}

devBase * devPCIE::createInstance(){
  return new devPCIE;
}

std::string devPCIE::createErrorStringWithErrnoText(std::string const & startText){
  char errorBuffer[255];
  return startText + _deviceName + ": " + strerror_r(errno, errorBuffer, sizeof(errorBuffer));
}

}//namespace mtca4u
