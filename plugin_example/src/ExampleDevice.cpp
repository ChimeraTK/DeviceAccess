/*
 * ExampleDevice.cpp
 *
 *  Created on: Jul 31, 2015
 *      Author: nshehzad
 */

#include "ExampleDevice.h"

#include <iostream>
using namespace mtca4u;

ExampleDevice::ExampleDevice(){
}

ExampleDevice::~ExampleDevice(){
  close();
}

boost::shared_ptr<mtca4u::DeviceBackend> ExampleDevice::createInstance(
  std::string /*host*/, std::string /*instance*/, std::list<std::string> /*parameters*/) {
  return boost::shared_ptr<mtca4u::DeviceBackend> ( new ExampleDevice );
}

void ExampleDevice::open(){
  _opened = true;
}

void ExampleDevice::close(){
  _opened = false;
}





