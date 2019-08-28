#include "Device.h"

#include <iostream>
#include <string>
#include <limits>
#include <chrono>

int main() {
  std::cout << "****************************************************************" << std::endl;
  std::cout << "*              Rebot Timeout Tests                             *" << std::endl;
  std::cout << "****************************************************************" << std::endl;

  std::string sdm;
  std::cout << "Enter Rebot device SDM: ";
  std::getline(std::cin, sdm);

  std::cout << sdm << std::endl;

  std::cout << std::endl;
  std::cout << std::endl;
  auto begin = std::chrono::system_clock::now();
  auto end = std::chrono::system_clock::now();
  unsigned long duration;
  std::string write_register;
  std::string read_register;
  /********************************************************************************/
  {
    std::cout << "Starting Test: Connection Timeout" << std::endl;
    std::cout << "Power down tmcb and press enter" << std::endl;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
    std::cout << "Trying to connect to TMCB..." << std::endl;
    ChimeraTK::Device d{sdm};
    begin = std::chrono::system_clock::now();
    try{
      d.open();
    } catch (ChimeraTK::runtime_error& e){
      end = std::chrono::system_clock::now(); 
      duration = std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
      std::cout << "Test Succesful: connection timed out after " << duration << " s" << std::endl;
    }
  }
  /********************************************************************************/
  std::cout << std::endl;
  std::cout << std::endl;
  std::cout << "Power on tmcb and press enter after it appears on the network" << std::endl;
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');

  /********************************************************************************/
  {
    std::cout << "Starting Test: Read Timeout" << std::endl;
    std::cout << "Please enter register name on the tmcb to read from:" << std::endl;
    std::getline(std::cin, read_register);
    ChimeraTK::Device d{sdm};
    d.open();
    std::cout << "Please pull ethernet cable and press enter..." << std::endl;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
    begin = std::chrono::system_clock::now();
    try{
      d.read<int>(read_register);
    } catch (ChimeraTK::runtime_error& e){
      end = std::chrono::system_clock::now(); 
      duration = std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
      std::cout << "Test Succesful: Read timed out after " << duration << " s" << std::endl;
    }
  }
  /********************************************************************************/

  std::cout << std::endl;
  std::cout << std::endl;
  std::cout << "Reconnect cable and powercycle tmcb; press enter after it appears on the network" << std::endl;
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');


  std::cout << std::endl;
  std::cout << std::endl;
  /********************************************************************************/
  {
    std::cout << "Starting Test: Write Timeout" << std::endl;
    std::cout << "Use same register as read for this test? Press enter if yes; else type new name" << std::endl; 
    std::getline(std::cin, write_register);
    
    if(write_register.empty()){
      write_register = read_register;
    }
  
    std::cout << write_register << " will be used for testing" << std::endl;
  
  
    ChimeraTK::Device d{sdm};
    d.open();
    std::cout << "Please pull ethernet cable and press enter..." << std::endl;
    std::getline(std::cin, read_register);
    begin = std::chrono::system_clock::now();
    try{
      d.write(write_register, 56);
    } catch (ChimeraTK::runtime_error& e){
      end = std::chrono::system_clock::now(); 
      duration = std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
      std::cout << "Test Succesful: Write timed out after " << duration << " s" << std::endl;
    }
  }
  /********************************************************************************/

  std::cout << "****************************************************************" << std::endl;
  std::cout << "*              Tests complete                                  *" << std::endl;
  std::cout << "****************************************************************" << std::endl;
}

