#include "devFake.h"
#include "exDevFake.h"
#include <iostream>
#include <algorithm>
#include <string.h>
#include <stdio.h>

devFake::devFake()
  : pcieMemory(0), pcieMemoryFileName()
{
    
}

devFake::~devFake()         
{
    closeDev();
}

void devFake::openDev(const std::string &devName, int /*perm*/, devConfigBase* /*pConfig*/)
{     
    std::string name = "./" + devName;    
    std::replace(name.begin(), name.end(), '/', '_');                
    pcieMemoryFileName = name;
    if (opened == true) {
        throw exDevFake("Device already has been opened", exDevFake::EX_DEVICE_OPENED);
    }
    pcieMemory = fopen(name.c_str(), "r+");
    if (pcieMemory == NULL){
        char zero[LIBDEV_BAR_MEM_SIZE] = {0};
        pcieMemory = fopen(name.c_str(), "w");
        if (pcieMemory == NULL){
            throw exDevFake("Cannot create fake device file", exDevFake::EX_CANNOT_CREATE_DEV_FILE);
        }                 
        for (int bar = 0; bar < LIBDEV_BAR_NR; bar++){
            if (fseek(pcieMemory, sizeof(zero) * bar, SEEK_SET) < 0){
                fclose(pcieMemory);
                throw exDevFake("Cannot init device memory file", exDevFake::EX_DEVICE_FILE_WRITE_DATA_ERROR);
            }
            if (fwrite(&zero, sizeof(zero), 1, pcieMemory) == 0){
                fclose(pcieMemory);
                throw exDevFake("Cannot init device memory file", exDevFake::EX_DEVICE_FILE_WRITE_DATA_ERROR);
            }
        }        
        fclose(pcieMemory);
        pcieMemory = fopen(name.c_str(), "r+");        
    }
    opened = true;
}

void devFake::closeDev()
{
    if (opened == true){
        fclose(pcieMemory);
    }
    opened = false;
}
    
void devFake::readReg(uint32_t regOffset, int32_t* data, uint8_t bar)
{   
    if (opened == false) {
        throw exDevFake("Device closed", exDevFake::EX_DEVICE_CLOSED);
    }  
    if (bar >= LIBDEV_BAR_NR) {
        throw exDevFake("Wrong bar number", exDevFake::EX_DEVICE_FILE_READ_DATA_ERROR);
    }
    if (regOffset >= LIBDEV_BAR_MEM_SIZE) {
        throw exDevFake("Wrong offset", exDevFake::EX_DEVICE_FILE_READ_DATA_ERROR);
    }
    
    if (fseek(pcieMemory, regOffset + LIBDEV_BAR_MEM_SIZE*bar, SEEK_SET) < 0){
        throw exDevFake("Cannot access memory file", exDevFake::EX_DEVICE_FILE_READ_DATA_ERROR);
    }
    
    if (fread(data, sizeof(int), 1, pcieMemory) == 0){
        throw exDevFake("Cannot read memory file", exDevFake::EX_DEVICE_FILE_READ_DATA_ERROR);
    }       
}

void devFake::writeReg(uint32_t regOffset, int32_t data, uint8_t bar)
{    
    if (opened == false) {
        throw exDevFake("Device closed", exDevFake::EX_DEVICE_CLOSED);
    }     
    if (bar >= LIBDEV_BAR_NR) {
        throw exDevFake("Wrong bar number", exDevFake::EX_DEVICE_FILE_WRITE_DATA_ERROR);
    }
    if (regOffset >= LIBDEV_BAR_MEM_SIZE) {
        throw exDevFake("Wrong offset", exDevFake::EX_DEVICE_FILE_WRITE_DATA_ERROR);
    }
    
    if (fseek(pcieMemory, regOffset + LIBDEV_BAR_MEM_SIZE*bar, SEEK_SET) < 0){
        throw exDevFake("Cannot access memory file", exDevFake::EX_DEVICE_FILE_WRITE_DATA_ERROR);
    }
    
    if (fwrite(&data, sizeof(int), 1, pcieMemory) == 0){
        throw exDevFake("Cannot write memory file", exDevFake::EX_DEVICE_FILE_WRITE_DATA_ERROR);
    }       
}
    
void devFake::readArea(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar)
{       
    if (opened == false) {
        throw exDevFake("Device closed", exDevFake::EX_DEVICE_CLOSED);
    }      
    if (size % 4) {
        throw exDevFake("Wrong data size - must be dividable by 4", exDevFake::EX_DEVICE_FILE_READ_DATA_ERROR);
    }        
    for (uint16_t i = 0; i < size/4; i++){
        readReg(regOffset + i*4, data + i, bar);
    }    
}

void devFake::writeArea(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar)
{       
    if (opened == false) {
        throw exDevFake("Device closed", exDevFake::EX_DEVICE_CLOSED);
    }      
    if (size % 4) {
        throw exDevFake("Wrong data size - must be dividable by 4", exDevFake::EX_DEVICE_FILE_WRITE_DATA_ERROR);
    } 
    for (uint16_t i = 0; i < size/4; i++){
        writeReg(regOffset + i*4, *(data + i), bar);        
    }
}
    
void devFake::readDMA(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar)
{   
    readArea(regOffset, data, size, bar);
}

void devFake::writeDMA(uint32_t regOffset, int32_t* data, size_t size, uint8_t bar)
{
    writeArea(regOffset, data, size, bar);    
}

void devFake::readDeviceInfo(std::string* devInfo)
{
    *devInfo = "fake device: " + pcieMemoryFileName;
}

