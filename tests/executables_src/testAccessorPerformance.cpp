#include "Device.h"
#include "Utilities.h"

using namespace mtca4u;

int main(){
  setDMapFilePath("dummies.dmap");

  Device device;
  device.open("PCIE1");

  auto accessor = device.getRegisterAccessor("WORD_STATUS","BOARD");

  for(int i= 0; i < 100000; ++i){
      accessor->read<int>();
  }

  return 0;
}
