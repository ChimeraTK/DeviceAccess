// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ExampleBackend.h"

#include <iostream>
using namespace ChimeraTK;

// The only instance of the backend registerer. It is instantiated when the
// library is loaded, which registers the signature of the backend to the
// factory.
ExampleBackend::BackendRegisterer ExampleBackend::backendRegisterer;

ExampleBackend::ExampleBackend() {
  FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl);
}

ExampleBackend::~ExampleBackend() {
  close();
}

boost::shared_ptr<ChimeraTK::DeviceBackend> ExampleBackend::createInstance(
    std::string /*address*/, std::map<std::string, std::string> /*parameters*/) {
  return boost::shared_ptr<ChimeraTK::DeviceBackend>(new ExampleBackend);
}

void ExampleBackend::open() {
  _opened = true;
}

void ExampleBackend::close() {
  _opened = false;
}

// We do not have a suitable buffering register accessor, so we throw an
// exception.
template<typename UserType>
boost::shared_ptr<NDRegisterAccessor<UserType>> ExampleBackend::getRegisterAccessor_impl(
    const ChimeraTK::RegisterPath& /*registerPathName*/, size_t /*wordOffsetInRegister*/, size_t /*numberOfWords*/,
    ChimeraTK::AccessModeFlags /*flags*/) {
  throw ChimeraTK::logic_error("Not implemented.");
}
