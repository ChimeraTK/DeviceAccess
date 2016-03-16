/*
 * ExampleDevice.cpp
 *
 *  Created on: Jul 31, 2015
 *      Author: nshehzad
 */

#include "ExampleBackend.h"

#include <iostream>
using namespace mtca4u;

// The only instance of the backend registerer. It is instantiated when the library is loaded, which 
// registers the signature of the backend to the factory.
ExampleBackend::BackendRegisterer ExampleBackend::backendRegisterer;

ExampleBackend::ExampleBackend(){
  FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getBufferingRegisterAccessor_impl);
  FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getTwoDRegisterAccessor_impl);
}

ExampleBackend::~ExampleBackend(){
  close();
}

boost::shared_ptr<mtca4u::DeviceBackend> ExampleBackend::createInstance(
  std::string /*host*/, std::string /*instance*/, std::list<std::string> /*parameters*/, std::string /*mapFileName*/) {
  return boost::shared_ptr<mtca4u::DeviceBackend> ( new ExampleBackend );
}

void ExampleBackend::open(){
  _opened = true;
}

void ExampleBackend::close(){
  _opened = false;
}

// We do not have a suitable buffering register accessor, so we throw an exception.
template<typename UserType>
boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > ExampleBackend::getBufferingRegisterAccessor_impl(
    const mtca4u::RegisterPath &/*registerPathName*/, size_t /*wordOffsetInRegister*/, size_t /*numberOfWords*/, bool /*enforceRawAccess*/) {
  throw mtca4u::DeviceException("Not implemented.", mtca4u::DeviceException::NOT_IMPLEMENTED);
}

// We do not have a suitable 2D register accessor, so we throw an exception.
template<typename UserType>
boost::shared_ptr< TwoDRegisterAccessorImpl<UserType> > ExampleBackend::getTwoDRegisterAccessor_impl(
    const mtca4u::RegisterPath & /*registerPathName*/) {
  throw mtca4u::DeviceException("Not implemented.", mtca4u::DeviceException::NOT_IMPLEMENTED);
}



