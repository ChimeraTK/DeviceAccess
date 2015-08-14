/*
 * ExampleDevice.h
 *
 *  Created on: Jul 31, 2015
 *      Author: nshehzad
 */

#ifndef SOURCE_DIRECTORY__EXAMPLES_EXAMPLEDEVICE_H_
#define SOURCE_DIRECTORY__EXAMPLES_EXAMPLEDEVICE_H_
#include "MtcaMappedDevice/BaseDeviceImpl.h"
#include "MtcaMappedDevice/DeviceConfigBase.h"
#include "MtcaMappedDevice/DeviceFactory.h"
#include <list>
#include <iostream>
#include <boost/shared_ptr.hpp>
using namespace mtca4u;

class ExampleDevice : public BaseDeviceImpl {
private:
	ExampleDevice(std::string host, std::string interface, std::list<std::string> parameters);
public:
  ExampleDevice(){};
  virtual ~ExampleDevice();
  virtual void openDev();
  virtual void closeDev();
  static boost::shared_ptr<mtca4u::BaseDevice> createInstance(std::string host, std::string interface, std::list<std::string> parameters);

  virtual void openDev(const std::string& /*devName*/, int /*perm*/,
                         DeviceConfigBase* /*pConfig*/) {};

    virtual void readReg(uint32_t /*regOffset*/, int32_t* /*data*/, uint8_t /*bar*/) {};
    virtual void writeReg(uint32_t /*regOffset*/, int32_t /*data*/, uint8_t /*bar*/) {};

    virtual void readArea(uint32_t /*regOffset*/, int32_t* /*data*/, size_t /*size*/,
                          uint8_t /*bar*/) {};
    virtual void writeArea(uint32_t /*regOffset*/, int32_t const* /*data*/, size_t /*size*/,
                           uint8_t /*bar*/) {};

    virtual void readDMA(uint32_t /*regOffset*/, int32_t* /*data*/, size_t /*size*/,
                         uint8_t /*bar*/) {};
    virtual void writeDMA(uint32_t /*regOffset*/, int32_t const* /*data*/, size_t /*size*/,
                          uint8_t /*bar*/) {};

    virtual void readDeviceInfo(std::string* /*devInfo*/) {};
};

class ExampleDeviceRegisterer{
public:
	ExampleDeviceRegisterer(){
#ifdef _DEBUG
		std::cout<<"ExampleDeviceRegisterer"<<std::endl;
#endif
		DeviceFactory::getInstance().registerDevice("exx","",&ExampleDevice::createInstance);
  }
/*	static ExampleDeviceRegisterer & init()
	{
		static ExampleDeviceRegisterer globalExampleDeviceRegisterer;
		return globalExampleDeviceRegisterer;
	}*/

};
ExampleDeviceRegisterer globalExampleDeviceRegisterer;


#endif /* SOURCE_DIRECTORY__EXAMPLES_EXAMPLEDEVICE_H_ */
