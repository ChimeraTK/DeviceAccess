/*
 * Application.cc
 *
 *  Created on: Jun 10, 2016
 *      Author: Martin Hierholzer
 */

#include <string>
#include <thread>

#include <boost/fusion/container/map.hpp>

#include <mtca4u/BackendFactory.h>

#include "Application.h"
#include "ApplicationModule.h"
#include "Accessor.h"
#include "DeviceAccessor.h"

using namespace ChimeraTK;

Application *Application::instance = nullptr;
std::mutex Application::instance_mutex;

/*********************************************************************************************************************/

Application::Application() {
  std::lock_guard<std::mutex> lock(instance_mutex);
  if(instance != nullptr) {
    throw std::string("Multiple instances of ChimeraTK::Application cannot be created."); // @todo TODO throw proper exception
  }
  instance = this;
}

/*********************************************************************************************************************/

void Application::run() {
  initialise();
  makeConnections();
  for(auto module : moduleList) {
    module->run();
  }
}

/*********************************************************************************************************************/

void Application::generateXML() {
  initialise();

}

/*********************************************************************************************************************/

void Application::connectAccessors(AccessorBase &a, AccessorBase &b) {
  VariableNetwork &network = findOrCreateNetwork(&a,&b);
  network.addAccessor(a);
  network.addAccessor(b);
}

/*********************************************************************************************************************/

void Application::publishAccessor(AccessorBase &a, const std::string& name) {
  VariableNetwork &network = findOrCreateNetwork(&a);
  network.addAccessor(a);
  if(a.isOutput()) {
    network.addDev2CSPublication(name);
  }
  else {
    network.addCS2DevPublication(a,name);
  }
}

/*********************************************************************************************************************/

void Application::connectAccessorToDevice(AccessorBase &a, const std::string &deviceAlias,
    const std::string &registerName, UpdateMode mode, size_t numberOfElements, size_t elementOffsetInRegister) {
  VariableNetwork &network = findOrCreateNetwork(&a);
  network.addAccessor(a);
  if(a.isOutput()) {
    network.addInputDeviceRegister(deviceAlias, registerName, mode, numberOfElements, elementOffsetInRegister);
  }
  else {
    network.addOutputDeviceRegister(deviceAlias, registerName, mode, numberOfElements, elementOffsetInRegister);
  }
}

/*********************************************************************************************************************/

boost::shared_ptr<mtca4u::ProcessVariable> Application::createDeviceAccessor(AccessorBase &a,
    const std::string &deviceAlias, const std::string &registerName, UpdateMode mode, size_t numberOfElements,
    size_t elementOffsetInRegister) {

  // open device if needed
  if(deviceMap.count(deviceAlias) == 0) {
    deviceMap[deviceAlias] = mtca4u::BackendFactory::getInstance().createBackend(deviceAlias);
    deviceMap[deviceAlias]->open();
  }

  // use wait_for_new_data mode if push update mode was requested
  mtca4u::AccessModeFlags flags{};
  if(mode == UpdateMode::push) flags = {AccessMode::wait_for_new_data};

  // create DeviceAccessor for the proper UserType
  // @todo TODO replace with boost::mpl::for_each loop!
  boost::shared_ptr<mtca4u::ProcessVariable> impl;
  if(a.getValueType() == typeid(int8_t)) {
    auto regacc = deviceMap[deviceAlias]->getRegisterAccessor<int8_t>(registerName,
        numberOfElements, elementOffsetInRegister, flags);
    impl.reset(new DeviceAccessor<int8_t>(regacc,a.getDirection(),mode));
  }
  else if(a.getValueType() == typeid(uint8_t)) {
    auto regacc = deviceMap[deviceAlias]->getRegisterAccessor<uint8_t>(registerName,
        numberOfElements, elementOffsetInRegister, flags);
    impl.reset(new DeviceAccessor<uint8_t>(regacc,a.getDirection(),mode));
  }
  else if(a.getValueType() == typeid(int16_t)) {
    auto regacc = deviceMap[deviceAlias]->getRegisterAccessor<int16_t>(registerName,
        numberOfElements, elementOffsetInRegister, flags);
    impl.reset(new DeviceAccessor<int16_t>(regacc,a.getDirection(),mode));
  }
  else if(a.getValueType() == typeid(uint16_t)) {
    auto regacc = deviceMap[deviceAlias]->getRegisterAccessor<uint16_t>(registerName,
        numberOfElements, elementOffsetInRegister, flags);
    impl.reset(new DeviceAccessor<uint16_t>(regacc,a.getDirection(),mode));
  }
  else if(a.getValueType() == typeid(int32_t)) {
    auto regacc = deviceMap[deviceAlias]->getRegisterAccessor<int32_t>(registerName,
        numberOfElements, elementOffsetInRegister, flags);
    impl.reset(new DeviceAccessor<int32_t>(regacc,a.getDirection(),mode));
  }
  else if(a.getValueType() == typeid(uint32_t)) {
    auto regacc = deviceMap[deviceAlias]->getRegisterAccessor<uint32_t>(registerName,
        numberOfElements, elementOffsetInRegister, flags);
    impl.reset(new DeviceAccessor<uint32_t>(regacc,a.getDirection(),mode));
  }
  else if(a.getValueType() == typeid(float)) {
    auto regacc = deviceMap[deviceAlias]->getRegisterAccessor<float>(registerName,
        numberOfElements, elementOffsetInRegister, flags);
    impl.reset(new DeviceAccessor<float>(regacc,a.getDirection(),mode));
  }
  else if(a.getValueType() == typeid(double)) {
    auto regacc = deviceMap[deviceAlias]->getRegisterAccessor<double>(registerName,
        numberOfElements, elementOffsetInRegister, flags);
    impl.reset(new DeviceAccessor<double>(regacc,a.getDirection(),mode));
  }
  return impl;
}

/*********************************************************************************************************************/

boost::shared_ptr<mtca4u::ProcessVariable> Application::createProcessScalar(AccessorBase &a, const std::string &name) {

  // determine the SynchronizationDirection
  SynchronizationDirection dir;
  if(a.isOutput()) {
    dir = SynchronizationDirection::deviceToControlSystem;
  }
  else {
    dir = SynchronizationDirection::controlSystemToDevice;
  }

  // create the ProcessScalar for the proper UserType
  // @todo TODO replace with boost::mpl::for_each loop!
  boost::shared_ptr<mtca4u::ProcessVariable> impl;
  if(a.getValueType() == typeid(int8_t)) {
    impl = _processVariableManager->createProcessScalar<int8_t>(dir, name);
  }
  else if(a.getValueType() == typeid(uint8_t)) {
    impl = _processVariableManager->createProcessScalar<uint8_t>(dir, name);
  }
  else if(a.getValueType() == typeid(int16_t)) {
    impl = _processVariableManager->createProcessScalar<int16_t>(dir, name);
  }
  else if(a.getValueType() == typeid(uint16_t)) {
    impl = _processVariableManager->createProcessScalar<uint16_t>(dir, name);
  }
  else if(a.getValueType() == typeid(int32_t)) {
    impl = _processVariableManager->createProcessScalar<int32_t>(dir, name);
  }
  else if(a.getValueType() == typeid(uint32_t)) {
    impl = _processVariableManager->createProcessScalar<uint32_t>(dir, name);
  }
  else if(a.getValueType() == typeid(float)) {
    impl = _processVariableManager->createProcessScalar<float>(dir, name);
  }
  else if(a.getValueType() == typeid(double)) {
    impl = _processVariableManager->createProcessScalar<double>(dir, name);
  }
  return impl;
}

/*********************************************************************************************************************/

std::pair< boost::shared_ptr<mtca4u::ProcessVariable>, boost::shared_ptr<mtca4u::ProcessVariable> >
  Application::createProcessScalar(AccessorBase &a) {

  // create the ProcessScalar for the proper UserType
  // @todo TODO replace with boost::mpl::for_each loop!
  if(a.getValueType() == typeid(int8_t)) {
    return createSynchronizedProcessScalar<int8_t>();
  }
  else if(a.getValueType() == typeid(uint8_t)) {
    return createSynchronizedProcessScalar<uint8_t>();
  }
  else if(a.getValueType() == typeid(int16_t)) {
    return createSynchronizedProcessScalar<int16_t>();
  }
  else if(a.getValueType() == typeid(uint16_t)) {
    return createSynchronizedProcessScalar<uint16_t>();
  }
  else if(a.getValueType() == typeid(int32_t)) {
    return createSynchronizedProcessScalar<int32_t>();
  }
  else if(a.getValueType() == typeid(uint32_t)) {
    return createSynchronizedProcessScalar<uint32_t>();
  }
  else if(a.getValueType() == typeid(float)) {
    return createSynchronizedProcessScalar<float>();
  }
  else if(a.getValueType() == typeid(double)) {
    return createSynchronizedProcessScalar<double>();
  }
  return std::pair< boost::shared_ptr<mtca4u::ProcessVariable>, boost::shared_ptr<mtca4u::ProcessVariable> >();
}

/*********************************************************************************************************************/

void Application::makeConnections() {

  // make connections between accessors
  for(auto network : networkList) {
    // make connection in single target case
    bool connectionMade = false;
    network.dump();
    if(network.countInputAccessors() == 0) {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalVariableNetwork>(
          "Illegal variable network found: no input accessors connected!");
    }
    if(network.countInputAccessors() == 1) {
      if(network.getOutputAccessorType() == VariableNetwork::AccessorType::Device) {
        if(network.hasInputAccessorType(VariableNetwork::AccessorType::Application)) {
          auto acc = network.getInputAccessorList().front(); // we have only one in the list here
          auto info = network.getDeviceOutputRegisterInfo();
          auto impl = createDeviceAccessor(*acc, info.deviceAlias, info.registerName, info.mode, 1, 0);
          acc->useProcessVariable(impl);
          connectionMade = true;
        }
      }
      else if(network.getOutputAccessorType() == VariableNetwork::AccessorType::Application) {
        if(network.hasInputAccessorType(VariableNetwork::AccessorType::Device)) {
          auto acc = network.getOutputAccessor();
          auto info = network.getDeviceInputRegisterInfos().front(); // we have only one in the list here
          auto impl = createDeviceAccessor(*acc, info.deviceAlias, info.registerName, info.mode, 1, 0);
          acc->useProcessVariable(impl);
          connectionMade = true;
        }
        else if(network.hasInputAccessorType(VariableNetwork::AccessorType::ControlSystem)) {
          auto acc = network.getOutputAccessor();
          auto pubName = network.getInputPublicationNames().front(); // we have only one in the list here
          auto impl = createProcessScalar(*acc,pubName);
          acc->useProcessVariable(impl);
          connectionMade = true;
        }
        else if(network.hasInputAccessorType(VariableNetwork::AccessorType::Application)) {
          auto accO = network.getOutputAccessor();
          auto accI = network.getInputAccessorList().front(); // we have only one in the list here
          auto impls = createProcessScalar(*accO);
          accO->useProcessVariable(impls.first);
          accI->useProcessVariable(impls.second);
          connectionMade = true;
        }
      }
      else if(network.getOutputAccessorType() == VariableNetwork::AccessorType::ControlSystem) {
        if(network.hasInputAccessorType(VariableNetwork::AccessorType::Application)) {
          auto acc = network.getInputAccessorList().front(); // we have only one in the list here
          auto pubName = network.getOutputPublicationName();
          auto impl = createProcessScalar(*acc,pubName);
          acc->useProcessVariable(impl);
          connectionMade = true;
        }
      }
    }
    if(!connectionMade) {
      throw ApplicationExceptionWithID<ApplicationExceptionID::notYetImplemented>(
          "The variable network cannot be handled. Implementation missing!"); // @todo TODO implement
    }
  }

  // make connections to devices
  /* for(auto devconn : deviceAccessorMap) {
    if(devconn.first->isInitialised()) {
      throw std::string("Connecting to multiple inputs is not yet implemented (2)."); // @todo TODO implement
    }
    devconn.first->useProcessVariable(devconn.second);
  } */

}

/*********************************************************************************************************************/

VariableNetwork& Application::findOrCreateNetwork(AccessorBase *a, AccessorBase *b) {

  // search for a and b in the inputAccessorList
  auto r = find_if(networkList.begin(), networkList.end(),
      [a,b](const VariableNetwork& n) { return n.hasAccessor(a,b); } );

  // if no network found, create one
  if(r == networkList.end()) {
    networkList.push_back({});
    return networkList.back();
  }

  // store the reference to the found network
  VariableNetwork &ret = *r;

  // check if more than one network is found
  if(++r != networkList.end()) {
    throw std::string("Trying to connect two accessors which already are part of a network."); // @todo TODO throw proper exception
  }

  // return the found network
  return ret;
}

/*********************************************************************************************************************/
