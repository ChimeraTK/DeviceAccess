#include <iostream>
#include <plugin/ExampleBackend.h>
#include <mtca4u/BackendFactory.h>
using namespace mtca4u;
int main() {
  //ExampleDeviceRegisterer::init();
  BackendFactory FactoryInstance = BackendFactory::getInstance();
  boost::shared_ptr<DeviceBackend> _pcieDeviceInstance;
  _pcieDeviceInstance = FactoryInstance.createBackend("PCIE0");
  if (_pcieDeviceInstance != 0)
  {
    //std::cout<<"Device Failed"<<std::endl;
    //return 1;

    if (_pcieDeviceInstance->isConnected() == true )
      std::cout<<"Device status: Connected"<<std::endl;

    if (_pcieDeviceInstance->isOpen() == false )
      std::cout<<"Device status: Closed"<<std::endl;
  }
  boost::shared_ptr<DeviceBackend> exampleDeviceInstance = FactoryInstance.createBackend("example");
  if (exampleDeviceInstance != 0)
  {
    //std::cout<<"Device Failed"<<std::endl;
    //return 1;

    if (exampleDeviceInstance->isConnected() == true )
      std::cout<<"Device status: Connected"<<std::endl;

    if (exampleDeviceInstance->isOpen() == false )
      std::cout<<"Device status: Closed"<<std::endl;

    exampleDeviceInstance->open();
    if (exampleDeviceInstance->isOpen() == true )
      std::cout<<"Device status: Open"<<std::endl;
    exampleDeviceInstance->readDeviceInfo();
    int *test = 0;
    exampleDeviceInstance->read(0,0,test,0);
    exampleDeviceInstance->write(0,0,0,0);
    exampleDeviceInstance->readDMA(0,0,test,0);
    exampleDeviceInstance->writeDMA(0,0,0,0);
    exampleDeviceInstance->close();
    if (exampleDeviceInstance->isOpen() == false )
      std::cout<<"Device status: Closed"<<std::endl;
  }

  return 0;
}

