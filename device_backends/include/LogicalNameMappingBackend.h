/*
 * LogicalNameMappingBackend.h
 *
 *  Created on: Feb 8, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_LOGICAL_NAME_MAPPING_BACKEND_H
#define MTCA4U_LOGICAL_NAME_MAPPING_BACKEND_H

#include "DeviceBackendImpl.h"

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

      virtual std::string readDeviceInfo() {
        return std::string("Logical name mapping file: ") + _lmapFileName;
      }

      static boost::shared_ptr<DeviceBackend> createInstance(std::string host, std::string instance,
          std::list<std::string> parameters, std::string mapFileName);

      virtual void read(const std::string &, const std::string &,
          int32_t *, size_t  = 0, uint32_t  = 0) {
        throw DeviceException("Not implemented", DeviceException::NOT_IMPLEMENTED);
      }

      virtual void write(const std::string &,
          const std::string &, int32_t const *,
          size_t  = 0, uint32_t  = 0)  {
        throw DeviceException("Not implemented", DeviceException::NOT_IMPLEMENTED);
      }

    protected:

      template<typename UserType>
      boost::shared_ptr< NDRegisterAccessor<UserType> > getRegisterAccessor_impl(
          const RegisterPath &registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, bool enforceRawAccess);
      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER( LogicalNameMappingBackend, getRegisterAccessor_impl, 4);

      /// parse the logical map file, if not yet done
      void parse();

      /// flag if already parsed
      bool hasParsed;

      /// name of the logical map file
      std::string _lmapFileName;

      /// map of target devices
      std::map< std::string, boost::shared_ptr<DeviceBackend> > _devices;

      template<typename T>
      friend class LNMBackendBufferingRegisterAccessor;

      template<typename T>
      friend class LNMBackendBufferingChannelAccessor;

      template<typename T>
      friend class LNMBackendBufferingVariableAccessor;

  };

}

#endif /* MTCA4U_LOGICAL_NAME_MAPPING_BACKEND_H */
