/*
 * ExampleDevice.cpp
 *
 *  Created on: Jul 31, 2015
 *      Author: nshehzad
 */

#include "ExampleBackend.h"

#include <iostream>
using namespace mtca4u;

ExampleBackend::ExampleBackend(){
}

ExampleBackend::~ExampleBackend(){
  close();
}

boost::shared_ptr<mtca4u::DeviceBackend> ExampleBackend::createInstance(
  std::string /*host*/, std::string /*instance*/, std::list<std::string> /*parameters*/) {
  return boost::shared_ptr<mtca4u::DeviceBackend> ( new ExampleBackend );
}

void ExampleBackend::open(){
  _opened = true;
}

void ExampleBackend::close(){
  _opened = false;
}





