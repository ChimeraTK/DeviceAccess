/*
 * ExampleBackend.h
 *
 *  Created on: Jul 31, 2015
 *      Author: nshehzad
 */

#ifndef SOURCE_DIRECTORY__EXAMPLES_EXAMPLEBACKEND_H_
#define SOURCE_DIRECTORY__EXAMPLES_EXAMPLEBACKEND_H_

#include <list>
#include <iostream>
#include <boost/shared_ptr.hpp>

#include <mtca4u/DeviceBackendImpl.h>
#include <mtca4u/BackendFactory.h>

/** An Example to show how to write a backend device class and add it to the factory.
 *
 */
class ExampleBackend : public mtca4u::DeviceBackendImpl {
  public:
    ExampleBackend();
    virtual ~ExampleBackend();
    virtual void open();
    virtual void close();
    static boost::shared_ptr<mtca4u::DeviceBackend> createInstance(std::string host, std::string instance,
        std::list<std::string> parameters, std::string mapFileName);

    virtual void read(uint8_t /*bar*/, uint32_t /*address*/, int32_t* /*data*/,  size_t /*sizeInBytes*/){};
    virtual void write(uint8_t /*bar*/, uint32_t /*address*/, int32_t const* /*data*/, size_t /*sizeInBytes*/) {};

    virtual void read(const std::string &/*regModule*/, const std::string &/*regName*/,
        int32_t */*data*/, size_t /*dataSize*/ = 0, uint32_t /*addRegOffset*/ = 0) {}
    virtual void write(const std::string &/*regName*/, const std::string &/*regModule*/, int32_t const */*data*/,
        size_t /*dataSize*/ = 0, uint32_t /*addRegOffset*/ = 0) {}

    virtual void readDMA(uint8_t /*bar*/, uint32_t /*address*/, int32_t* /*data*/, size_t /*sizeInBytes*/) {};
    virtual void writeDMA(uint8_t /*bar*/, uint32_t /*address*/, int32_t const* /*data*/, size_t /*sizeInBytes*/) {};
    virtual std::string readDeviceInfo() {return std::string("Example_Device");}

    virtual boost::shared_ptr<mtca4u::RegisterAccessor> getRegisterAccessor(
        const std::string &/*registerName*/, const std::string &/*module*/ = std::string()) {
      return boost::shared_ptr<mtca4u::RegisterAccessor>();
    }

    virtual boost::shared_ptr<const mtca4u::RegisterInfoMap> getRegisterMap() const {
      return boost::shared_ptr<const mtca4u::RegisterInfoMap>();
    }

    virtual std::list<mtca4u::RegisterInfoMap::RegisterInfo> getRegistersInModule(
        const std::string &/*moduleName*/) const {
      return std::list<mtca4u::RegisterInfoMap::RegisterInfo>();
    }

    virtual std::list< boost::shared_ptr<mtca4u::RegisterAccessor> > getRegisterAccessorsInModule(
        const std::string &/*moduleName*/) {
      return std::list< boost::shared_ptr<mtca4u::RegisterAccessor> >();
    }

  protected:

    virtual void* getRegisterAccessor2Dimpl(const std::type_info &/*UserType*/, const std::string &/*dataRegionName*/,
        const std::string &/*module*/) {
      return NULL;
    }

  public:

    /** The registerer is announcing the new type to the registerer in its constructor.
     *  We have one static instance of this registerer in the backend. This causes the
     *  constructor to be executed when the library is loaded, end the backend is known by the
     *  factory afterwards.
     */
    class BackendRegisterer{
    public:
      BackendRegisterer(){
        mtca4u::BackendFactory::getInstance().registerBackendType("exx","",&ExampleBackend::createInstance);
      }
    };
    /** The one static instance of the registerer. Currently we keep it public so there is an object which
     *  can be used in the client code. This trick is needed to force the library to be loaded
     *  as long as the loading mechanism is not implemented into the dmap file
     *  (see DeviceClient.cpp how to do it).
     */
    static BackendRegisterer backendRegisterer;
};

#endif /* SOURCE_DIRECTORY__EXAMPLES_EXAMPLEBACKEND_H_ */
