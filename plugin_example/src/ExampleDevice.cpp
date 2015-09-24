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
ExampleDevice::ExampleDevice(){
#ifdef _DEBUG
	std::cout<<"ExampleDevice is connected"<<std::endl;
#endif
}

ExampleDevice::~ExampleDevice() {
	close();
}

boost::shared_ptr<mtca4u::DeviceBackend> ExampleDevice::createInstance(
    std::string host, std::string instance, std::list<std::string> parameters) {
#ifdef _DEBUG
	std::cout<<"example createInstance"<<std::endl;
#endif
	return boost::shared_ptr<mtca4u::DeviceBackend> ( new ExampleDevice );
}

void ExampleDevice::open()
{
#ifdef _DEBUG
	std::cout<<"open example device"<<std::endl;
#endif
}

void ExampleDevice::close()
{
	_opened = false;
}





