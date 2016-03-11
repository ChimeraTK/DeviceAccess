/*
 * ExampleDevice.cpp
 *
 *  Created on: Jul 31, 2015
 *      Author: nshehzad
 */

#include "ExampleBackend.h"
#include "BackendBufferingRegisterAccessor.h"

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

// Return the standard BackendBufferingRegisterAccessor. The behaviour would be the same if we would not implement
// this function, as it is already implemented like this in the base class! This default accessor is suitable
// whenever we can rely on the (non-buffering) RegisterAccessor to perform the actual access.
template<typename UserType>
boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > ExampleBackend::getBufferingRegisterAccessor_impl(
    const std::string &registerName, const std::string &module) {
  return boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> >(
      new BackendBufferingRegisterAccessor<UserType>(shared_from_this(),registerName,module) );
}

// We do not have a suitable 2D register accessor, so we throw an exception.
template<typename UserType>
boost::shared_ptr< TwoDRegisterAccessorImpl<UserType> > ExampleBackend::getTwoDRegisterAccessor_impl(
    const std::string & /*registerName*/, const std::string & /*module*/) {
  throw mtca4u::DeviceException("Not implemented.", mtca4u::DeviceException::NOT_IMPLEMENTED);
}



