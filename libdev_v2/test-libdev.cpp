#include "devPCIE.h"
#include "devFake.h"
#include "exBase.h"
#include <cstdlib>
#include <iostream>
#include <string.h>


using namespace std;



/*
 * 
 */
int main(int /*argc*/, char** /*argv*/) 
{ 
    //devFake               dev;
    devPCIE               dev;
    int32_t               data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};      
    size_t                data_size = 10 * 4;
    
    try {
    
        dev.openDev("/dev/mtcadummys0");   

        dev.writeArea(0, data, data_size, 0);
        dev.closeDev();

        memset(data, 0, sizeof(data));

        dev.openDev("/dev/mtcadummys0");

        dev.readArea(0, data, data_size, 0);

        for (unsigned int i = 0; i < data_size/4; i++){
            std::cout << std::hex << data[i] << " ";
        }
        std::cout << std::dec << std::endl;    
        dev.closeDev();
    
    } catch (exBase &e) {
        std::cout << e.what() << std::endl;
    }    
    return 0;
}

