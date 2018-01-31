#ifndef CHIMERATK_SUBDEVICE_BACKEND_H
#define CHIMERATK_SUBDEVICE_BACKEND_H

#include <string>
#include <stdint.h>
#include <fcntl.h>
#include <vector>

#include "DeviceBackendImpl.h"

namespace ChimeraTK {

  /**
   *  Backend for subdevices which are passed through some register or area of another device. The subdevice is close
   *  to a numeric addressed backend and has a map file of the same format. The other device may be of any type.
   *
   *  The sdm URI syntax for setting up the subdevice depends on the protocol used to pass through the registers.
   *  Currently only the "area" type is supported, which uses a 1D register as an address space. Bars other than bar 0
   *  are not supported.
   *
   *  URI scheme for the "area" type:
   *
   *    sdm://./subdevice:area,<targetDevice>,<targetRegister>=<mapFile>
   *
   *  Example: We like to use the register "APP.0.EXT_PZ16M" of the device with the alias name "TCK7_0" in our dmap
   *  file as a target and the file piezo_pz16m_acc1_r0.mapp as a map file. The file contains addresses relative to the
   *  beginning of the register "APP.0.EXT_PZ16M". The URI then looks like this:
   *
   *    sdm://./subdevice:area,TCK7_0,APP.0.EXT_PZ16M=piezo_pz16m_acc1_r0.mapp
   */
  class SubdeviceBackend : public DeviceBackendImpl {

    public:

      SubdeviceBackend(std::string instance, std::string mapFileName);

      ~SubdeviceBackend(){}

      void open() override;

      void close() override;

      std::string readDeviceInfo() override {
        return std::string("Subdevice");  /// @todo extend information
      }

      static boost::shared_ptr<DeviceBackend> createInstance(std::string host, std::string instance,
          std::list<std::string> parameters, std::string mapFileName);

    protected:

      enum class Type {
        area
      };

      /// type of the subdeivce
      Type type;

      /// the target device name
      std::string targetAlias;

      /// The target device backend itself. We are using directly a backend since we want to obtain
      //  NDRegisterAccessors which we can directly return in getRegisterAccessor_impl().
      boost::shared_ptr<mtca4u::DeviceBackend> targetDevice;

      /// for type == area: the name of the target register
      std::string targetArea;

      /// map from register names to addresses
      boost::shared_ptr<RegisterInfoMap> _registerMap;

      template<typename UserType>
      boost::shared_ptr< NDRegisterAccessor<UserType> > getRegisterAccessor_impl(
          const RegisterPath &registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);
      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER( SubdeviceBackend, getRegisterAccessor_impl, 4 );

  };

} // namespace ChimeraTK

#endif /*CHIMERATK_SUBDEVICE_BACKEND_H*/

