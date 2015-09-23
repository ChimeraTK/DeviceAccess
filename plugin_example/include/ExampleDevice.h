/*
 * ExampleDevice.h
 *
 *  Created on: Jul 31, 2015
 *      Author: nshehzad
 */

#ifndef SOURCE_DIRECTORY__EXAMPLES_EXAMPLEDEVICE_H_
#define SOURCE_DIRECTORY__EXAMPLES_EXAMPLEDEVICE_H_
#include <mtca4u-deviceaccess/DeviceBackendImpl.h>
#include <mtca4u-deviceaccess/BackendFactory.h>
#include <list>
#include <iostream>
#include <boost/shared_ptr.hpp>
using namespace mtca4u;

/** An Example to show how to write a device class and add it to the factory.
 *
 */
class ExampleDevice : public DeviceBackendImpl {
private:
	ExampleDevice(std::string host, std::string instance, std::list<std::string> parameters);
public:
  ExampleDevice(){};
  virtual ~ExampleDevice();
  virtual void open();
  virtual void close();
  static boost::shared_ptr<mtca4u::DeviceBackend> createInstance(std::string host, std::string instance, std::list<std::string> parameters);

  virtual void read(uint8_t /*bar*/, uint32_t /*address*/, int32_t* /*data*/,  size_t /*sizeInBytes*/){};
  virtual void write(uint8_t /*bar*/, uint32_t /*address*/, int32_t const* /*data*/, size_t /*sizeInBytes*/) {};

  virtual void readDMA(uint8_t /*bar*/, uint32_t /*address*/, int32_t* /*data*/, size_t /*sizeInBytes*/) {};
  virtual void writeDMA(uint8_t /*bar*/, uint32_t /*address*/, int32_t const* /*data*/, size_t /*sizeInBytes*/) {};
  virtual std::string readDeviceInfo() {return std::string("Example_Device");}
};

/** This class is used as a way to register the device to the factory.
 *
 */
class ExampleDeviceRegisterer{
public:
	ExampleDeviceRegisterer(){
#ifdef _DEBUG
		std::cout<<"ExampleDeviceRegisterer"<<std::endl;
#endif
		BackendFactory::getInstance().registerDeviceType("exx","",&ExampleDevice::createInstance);
  }
};
ExampleDeviceRegisterer globalExampleDeviceRegisterer;


#endif /* SOURCE_DIRECTORY__EXAMPLES_EXAMPLEDEVICE_H_ */
