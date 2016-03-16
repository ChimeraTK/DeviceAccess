#include <mtca4u/Device.h>
#include <mtca4u/TwoDRegisterAccessor.h>
#include <mtca4u/BackendFactory.h>
#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>

static const std::string MODULE_NAME = "ADC";
static const std::string DATA_REGION_NAME = "DATA";
// This register allows direct access to the multiplexed data
static const std::string REGISTER_TO_SETUP_DATA_REGION = "AREA_DATA_RAW";

int main() {
  mtca4u::BackendFactory::getInstance().setDMapFilePath("example.dmap");
  mtca4u::Device myDevice;
  myDevice.open("MY_DEVICE");

  /* We populate the memory region with multiple multiplexec sequences
   * so that we can use this for demonstrating the demultiplexing of the
   * TwoDRegisterAccessor (for some implementations depeding on the backend).
   * 
   * In this example we only have 4 sequences with 4 samples each.
   * We write numbers 0 to 15 as multiplexed data and expect the following result:
   * sequence 0:  0   4   8   12
   * sequence 1:  1   5   9   13
   * sequence 2:  2   6   10  14
   * sequence 3:  3   7   11  15
   */

  auto dataRegion 
    = myDevice.getBufferingRegisterAccessor<double>(MODULE_NAME,
						  REGISTER_TO_SETUP_DATA_REGION);
  int counter = 0;
  for (auto & dataWord : dataRegion){
    dataWord=counter++;
  }
  dataRegion.write();
  
  // Now check how it looks using the TwoDRegisterAccessor. We just copy it from
  // the accessor2D.cpp example. 
   mtca4u::TwoDRegisterAccessor<double> twoDAccessor =
    myDevice.getTwoDRegisterAccessor<double>(MODULE_NAME, DATA_REGION_NAME);
  twoDAccessor.read();

  for (size_t i = 0; i < twoDAccessor.getNumberOfDataSequences(); ++i){
    std::cout << "Channel " << i << ":";
    std::vector<double> & channel = twoDAccessor[i];
    for (double sample : channel){
      std::cout << " " << sample;
    }
    std::cout << std::endl;
  }

  myDevice.close();

  return 0;
}
