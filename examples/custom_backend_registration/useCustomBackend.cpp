#include <mtca4u/Device.h>
#include <mtca4u/Utilities.h>

int main(){
  mtca4u::setDMapFilePath("registration_example.dmap");
  // The device in known now because it is specified in the dmap file.

  mtca4u::Device myCustomDevice;
  myCustomDevice.open("MY_CUSTOM_DEVICE");

  return 0;
}
 
  

