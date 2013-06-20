/**
 *      @file           test-libptime.cpp
 *      @author         Adam Piotrowski <adam.piotrowski@desy.de>
 *      @version        1.0
 *      @brief          Testbench for libptime class               
 */

#include <cstdlib>
#include <iostream>
#include <iomanip>      
#include <sstream>
#include "include/libptime.h"
#include "../../lib/include/dbg_print.h"

void printInfo(const std::string& msg) {
    std::ostringstream os;
    os << "=" << std::setw(msg.length() + (91 - msg.length()) / 2) << std::right << msg << std::setw((91 - msg.length()) / 2 + (msg.length() % 2 ? 0 : 1)) << std::right << "=";
    std::cout << "============================================================================================" << std::endl;
    std::cout << os.str().c_str() << std::endl;
    std::cout << "============================================================================================" << std::endl;
}

int main(int argc, char** argv) 
{
    /*=============================================================================================================*/    
    timePeriod tp;
    printInfo("Starting time measurement (sleep 1)");
    /** [TIME time measurement example] */
    tp.tp_start();
    sleep(1);
    tp.tp_stop();
    std::cout << "TIME: " << tp.tp_getperiod(timePeriod::US) << std::endl;
    printInfo("Time measurement has finished");
    /** [TIME time measurement example] */    
    /*=============================================================================================================*/
    return 0;
}

