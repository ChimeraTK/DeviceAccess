#include "libdev_fake.h"
#include <iostream>
#include <algorithm>
#include <string.h>
#include <stdio.h>

devAccess_Fake::devAccess_Fake()         
{
    
}

devAccess_Fake::~devAccess_Fake()         
{
    closeDev();
}

uint16_t devAccess_Fake::openDev(const std::string &_dev_name, int perm, devConfig_Base* pconfig)
{ 
    std::string name = _dev_name;    
    std::replace(name.begin(), name.end(), '/', '_');        
    pcie_memory = fopen(("./" + name).c_str(), "r+");    
    if (pcie_memory == NULL){
        char zero[LIBDEV_BAR_MEM_SIZE] = {0};
        pcie_memory = fopen(("./" + name).c_str(), "w");
        if (pcie_memory == NULL){
           SET_STATUS(STATUS_FAILURE, "Cannot create device memory file"); 
        }                
        for (int bar = 0; bar < LIBDEV_BAR_NR; bar++){
            if (fseek(pcie_memory, sizeof(zero) * bar, SEEK_SET) < 0){
                fclose(pcie_memory);
                SET_STATUS(STATUS_FAILURE, "Cannot fill device memory file");
            }
            if (fwrite(&zero, sizeof(zero), 1, pcie_memory) == 0){
                fclose(pcie_memory);
                SET_STATUS(STATUS_FAILURE, "Cannot fill device memory file"); 
            }
        }        
        fclose(pcie_memory);
        pcie_memory = fopen(("./" + name).c_str(), "r+");
    }
    SET_STATUS(STATUS_OK, "OK");
}

uint16_t devAccess_Fake::closeDev()
{
    if (status == STATUS_OK){
        fclose(pcie_memory);
    }
    SET_STATUS(STATUS_CLOSED, "Device closed");
}
    
uint16_t devAccess_Fake::readReg(uint32_t reg_offset, int32_t* data, uint8_t bar)
{   
    if (status != STATUS_OK) SET_STATUS(STATUS_CLOSED, "Device closed");    
    if (bar >= LIBDEV_BAR_NR) {
        SET_LAST_ERROR(READ_FAILURE, "Wrong bar number"); 
        return READ_FAILURE;
    }
    if (reg_offset >= LIBDEV_BAR_MEM_SIZE) {
        SET_LAST_ERROR(READ_FAILURE, "Wrong offset"); 
        return READ_FAILURE;
    }
    
    if (fseek(pcie_memory, reg_offset + LIBDEV_BAR_MEM_SIZE*bar, SEEK_SET) < 0){
        SET_LAST_ERROR(READ_FAILURE, "Cannot access memory file"); 
        return READ_FAILURE;
    }
    
    if (fread(data, sizeof(int), 1, pcie_memory) == 0){
        SET_LAST_ERROR(READ_FAILURE, "Cannot read memory file"); 
        return READ_FAILURE; 
    }
    
    SET_LAST_ERROR(STATUS_OK, "OK");         
}

uint16_t devAccess_Fake::writeReg(uint32_t reg_offset, int32_t data, uint8_t bar)
{    
    if (status != STATUS_OK) SET_STATUS(STATUS_CLOSED, "Device closed");    
    if (bar >= LIBDEV_BAR_NR) {
        SET_LAST_ERROR(READ_FAILURE, "Wrong bar number"); 
        return READ_FAILURE;
    }
    if (reg_offset >= LIBDEV_BAR_MEM_SIZE) {
        SET_LAST_ERROR(READ_FAILURE, "Wrong offset"); 
        return READ_FAILURE;
    }
    
    if (fseek(pcie_memory, reg_offset + LIBDEV_BAR_MEM_SIZE*bar, SEEK_SET) < 0){
        SET_LAST_ERROR(READ_FAILURE, "Cannot access memory file"); 
        return READ_FAILURE;
    }
    
    if (fwrite(&data, sizeof(int), 1, pcie_memory) == 0){
        SET_LAST_ERROR(READ_FAILURE, "Cannot write memory file"); 
        return READ_FAILURE; 
    }
    
    SET_LAST_ERROR(STATUS_OK, "OK");        
}
    
uint16_t devAccess_Fake::readArea(uint32_t reg_offset, int32_t* data, size_t &size, uint8_t bar)
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

uint16_t devAccess_Fake::writeArea(uint32_t reg_offset, int32_t* data, size_t &size, uint8_t bar)
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
    
uint16_t devAccess_Fake::readDMA(uint32_t reg_offset, int32_t* data, size_t &size, uint8_t bar)
{   
        return readArea(reg_offset, data, size, bar);
}

uint16_t devAccess_Fake::writeDMA(uint32_t reg_offset, int32_t* data, size_t &size, uint8_t bar)
{
    SET_LAST_ERROR(NOT_SUPPORTED, "Operation not supported yet");
}

