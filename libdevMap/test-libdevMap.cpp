#include "exBase.h"
#include "devFake.h"
#include "devPCIE.h"
#include "devMap.h"
#include <iostream>
#include <string.h>

#include "libdevMap.h"

using namespace std;
        

typedef devMap<devFake>                 devMapFake;
typedef devMap<devFake>::regObject      devMapFakeRegObj;

typedef devMap<devPCIE>                 devMapPCIE;
typedef devMap<devPCIE>::regObject      devMapPCIERegObj;

int main(int /*argc*/, char** /*argv*/) 
{ 
 
    int32_t     data[255];
    
    try {       
        
        
        devMapFake dev;
        dev.openDev("/dev/pcieFakeDev", "./sis_demo_1.map");
    
        for (int i = 0; i < 8; i++){
            data[i] = i;
        }
        
        dev.writeReg("WORD_CAV_LIMIT", data);
        memset(data, 0, sizeof(int32_t)*8);
        dev.readReg("WORD_CAV_LIMIT", data);
        for (int i = 0; i < 8; i++){
                std::cout << std::hex << "0x" << data[i] << " " << std::dec;
        }
        std::cout << std::endl;
        memset(data, 0, sizeof(int32_t)*8);
        
        dev.readArea(0x000004B8, data, sizeof(int32_t)*8, 1);        
        for (int i = 0; i < 8; i++){
                std::cout << std::hex << "0x" << data[i] << " " << std::dec;
        }
        std::cout << std::endl;
        
        data[0] = 0xF1;
        data[1] = 0x1F;
        dev.writeReg("WORD_CAV_LIMIT", data, 8, 16);
        memset(data, 0, sizeof(int32_t)*8);
        dev.readReg("WORD_CAV_LIMIT", data);
        for (int i = 0; i < 8; i++){
                std::cout << std::hex << "0x" << data[i] << " " << std::dec;
        }
        std::cout << std::endl;
        
        
        devMapFakeRegObj ro = dev.getRegObject("WORD_CAV_LIMIT");
        memset(data, 0, sizeof(int32_t)*8);
        ro.readReg(data);
        for (int i = 0; i < 8; i++){
                std::cout << std::hex << "0x" << data[i] << " " << std::dec;
        }
        std::cout << std::endl;
        
    } catch (exBase &e) {
        std::cout << e.what() << std::endl;
    }    
    return 0;
}

