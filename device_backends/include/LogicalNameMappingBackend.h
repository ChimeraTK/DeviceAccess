/*
 * LogicalNameMappingBackend.h
 *
 *  Created on: Feb 8, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_LOGICAL_NAME_MAPPING_BACKEND_H
#define MTCA4U_LOGICAL_NAME_MAPPING_BACKEND_H

#include "DeviceBackendImpl.h"
#include "LogicalNameMap.h"

namespace mtca4u {

  /** Backend to map logical register names onto real hardware registers. It reads the logical name map from an xml
   *  file and will open internally additional Devices as they are reference in that file.
   */
  class LogicalNameMappingBackend : public DeviceBackendImpl {

    public:

      LogicalNameMappingBackend(std::string lmapFileName="");

      virtual ~LogicalNameMappingBackend(){}

      virtual void open();

      virtual void close();

      virtual void read(const std::string &regModule, const std::string &regName,
          int32_t *data, size_t dataSize = 0, uint32_t addRegOffset = 0);

      virtual void write(const std::string &regModule, const std::string &regName,
          int32_t const *data, size_t dataSize = 0, uint32_t addRegOffset = 0);

      virtual std::string readDeviceInfo() {
        return std::string("Logical name mapping file: ") + _lmapFileName;
      }

      virtual boost::shared_ptr<mtca4u::RegisterAccessor> getRegisterAccessor(
          const std::string &/*registerName*/,
          const std::string &/*module*/ = std::string());

      virtual std::list<mtca4u::RegisterInfoMap::RegisterInfo> getRegistersInModule(
          const std::string &/*moduleName*/) const {
        throw DeviceException("Not yet implemented.",DeviceException::NOT_IMPLEMENTED);
      }

      virtual std::list< boost::shared_ptr<mtca4u::RegisterAccessor> > getRegisterAccessorsInModule(
          const std::string &/*moduleName*/) {
        throw DeviceException("Not yet implemented.",DeviceException::NOT_IMPLEMENTED);
      }

      static boost::shared_ptr<DeviceBackend> createInstance(std::string host, std::string instance,
          std::list<std::string> parameters, std::string mapFileName);

    protected:

      VIRTUAL_FUNCTION_TEMPLATE_DECLARATION(getTwoDRegisterAccessorImpl, const std::string &, const std::string &);

      VIRTUAL_FUNCTION_TEMPLATE_DECLARATION(getBufferingRegisterAccessorImpl, const std::string &, const std::string &);

      template<typename UserType>
      TwoDRegisterAccessorImpl<UserType>* getTwoDRegisterAccessor(const std::string &module,
          const std::string &registerName);

      template<typename UserType>
      BufferingRegisterAccessorImpl<UserType>* getBufferingRegisterAccessor(const std::string &module,
          const std::string &registerName);

      /// name of the logical map file
      std::string _lmapFileName;

      /// logical name map
      LogicalNameMap _map;

      /// map of target devices
      std::map< std::string, boost::shared_ptr<Device> > _devices;

      template<typename T>
      friend class LogicalNameMappingBackendBufferingRangeRegisterAccessor;

  };

}

#endif /* MTCA4U_LOGICAL_NAME_MAPPING_BACKEND_H */
