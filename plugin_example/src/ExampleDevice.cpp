/*
 * ExampleDevice.cpp
 *
 *  Created on: Jul 31, 2015
 *      Author: nshehzad
 */

#include "ExampleDevice.h"

#include <iostream>
using namespace mtca4u;
#define _DEBUG
ExampleDevice::ExampleDevice(std::string host, std::string interface, std::list<std::string> parameters)
: BaseDeviceImpl(host,interface,parameters)
{
#ifdef _DEBUG
	std::cout<<"ExampleDevice is connected"<<std::endl;
#endif
}
ExampleDevice::~ExampleDevice() {
	closeDev();
}
boost::shared_ptr<mtca4u::BaseDevice> ExampleDevice::createInstance(std::string host, std::string interface, std::list<std::string> parameters) {
#ifdef _DEBUG
	std::cout<<"example createInstance"<<std::endl;
#endif
	return boost::shared_ptr<BaseDevice> ( new ExampleDevice(host,interface,parameters) );
}
void ExampleDevice::openDev()
{
#ifdef _DEBUG
	std::cout<<"open example device"<<std::endl;
#endif
}

void ExampleDevice::closeDev()
{
	_opened = false;
}





