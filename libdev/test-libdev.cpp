/* 
 * File:   main.cpp
 * Author: apiotro
 *
 * Created on 17 maj 2011, 10:53
 */

#include <cstdlib>
#include <iostream>
#include <string.h>
#include "include/libdev_struct.h"
#include "include/libdev_multifile.h"
#include "include/libdev_fake.h"
#include "include/libdev_serial.h"

using namespace std;



/*
 * 
 */
int main(int argc, char** argv) 
{
    
    devAccess_Serial dev_s;
    devConfig_Serial conf_s;
    devConfig_Base conf_b;
    if (dev_s.openDev("/dev/ttyS0", O_RDWR, &conf_s) != STATUS_OK){
        std::cout << dev_s.getLastErrorString() << std::endl;
        return -1;
    }
    
    return 0;
    
    //devAccess_Struct    dev;
    devAccess_Fake   dev;
    int32_t               data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};      
    size_t                data_size = 10 * 4;
    
    
    if (dev.openDev("/dev/pcie_bar_0") != STATUS_OK){
        std::cout << dev.getLastErrorString() << std::endl;
        return -1;
    }    
    
    if (dev.writeArea(0, data, data_size, 0) != STATUS_OK){
        std::cout << dev.getLastErrorString() << std::endl;
        return -1;
    }
    dev.closeDev();
    
    memset(data, 0, sizeof(data));
    
    if (dev.openDev("/dev/pcie_bar_0") != STATUS_OK){
        std::cout << dev.getLastErrorString() << std::endl;
        return -1;
    }
    
    if (dev.readArea(0, data, data_size, 0) != STATUS_OK){
        std::cout << dev.getLastErrorString() << std::endl;
        return -1;
    }
    
    for (unsigned int i = 0; i < data_size/4; i++){
        std::cout << std::hex << data[i] << " ";
    }
    std::cout << std::dec << std::endl;    
    dev.closeDev();
    
    
    
    return 0;
 /*   
    if (dev.openDev("/dev/pcie_dma_0", O_RDONLY) != STATUS_OK){
        std::cout << dev.getLastErrorString() << std::endl;
        return -1;
    }
    
    
    std::cout << data_size << std::endl;
    if (dev.readDMA(0, data, data_size) != STATUS_OK){
        std::cout << dev.getLastErrorString() << std::endl;
        return -1;
    }
    std::cout << data_size << std::endl;
    for (unsigned int i = 0; i < data_size/4; i++){
        std::cout << std::hex << data[i] << " ";
    }
    std::cout << std::endl;
   */ 
    return 0;
}

