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
   *  The following pass-trough types are supported:
   *
   *  "area" type:  use a 1D register as an address space. Bars other than bar 0 are not supported.
   *                URI scheme:
   *                  (subdevice?type=area&device=\<targetDevice\>&area=\<targetRegister\>&map=\<mapFile\>)
   *
   *  "3regs" type: use three scalar registers: address, data and status. Before access, a value of 0 in the status
   *                register is awaited. Next, the address is written to the address register. The value is then written
   *                to resp. read from the data register. Bars other than bar 0 are not supported.
   *                URI scheme:
   *                  (subdevice?type=3regs&device=\<targetDevice\>&address=\<addressRegister\>&data=\<dataRegister\>&status=\<statusRegister\>&map=\<mapFile\>)
   *
   *  Example: We like to use the register "APP.0.EXT_PZ16M" of the device with the alias name "TCK7_0" in our dmap
   *  file as a target and the file piezo_pz16m_acc1_r0.mapp as a map file. The file contains addresses relative to the
   *  beginning of the register "APP.0.EXT_PZ16M". The URI then looks like this:
   *
   *    (subdevice?type=area&device=TCK7_0&area=APP.0.EXT_PZ16M&map=piezo_pz16m_acc1_r0.mapp)
   *
   */
  class SubdeviceBackend : public DeviceBackendImpl {

    public:

      SubdeviceBackend(std::map<std::string,std::string> parameters);

      ~SubdeviceBackend(){}

      void open() override;

      void close() override;

      std::string readDeviceInfo() override {
        return std::string("Subdevice");  /// @todo extend information
      }

      static boost::shared_ptr<DeviceBackend> createInstance(std::string address, std::map<std::string,std::string> parameters);

    protected:

      enum class Type {
        area,               // address space is visible as an area in the target device
        threeRegisters      // use three registers (address, data and status) in target device. status must be 0 when idle
      };

      /// type of the subdeivce
      Type type;

      /// the target device name
      std::string targetAlias;

      /// The target device backend itself. We are using directly a backend since we want to obtain
      //  NDRegisterAccessors which we can directly return in getRegisterAccessor_impl().
      boost::shared_ptr<ChimeraTK::DeviceBackend> targetDevice;

      /// for type == area: the name of the target register
      std::string targetArea;

      /// for type == threeRegisters: the name of the target registers
      std::string targetAddress, targetData, targetControl;

      /// map from register names to addresses
      boost::shared_ptr<RegisterInfoMap> _registerMap;

      template<typename UserType>
      boost::shared_ptr< NDRegisterAccessor<UserType> > getRegisterAccessor_impl(
          const RegisterPath &registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);
      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER( SubdeviceBackend, getRegisterAccessor_impl, 4 );

      /// getRegisterAccessor implemenation for area types
      template<typename UserType>
      boost::shared_ptr< NDRegisterAccessor<UserType> > getRegisterAccessor_area(
          const RegisterPath &registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);

      /// getRegisterAccessor implemenation for threeRegisters types
      template<typename UserType>
      boost::shared_ptr< NDRegisterAccessor<UserType> > getRegisterAccessor_3regs(
          const RegisterPath &registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);

  };

} // namespace ChimeraTK

#endif /*CHIMERATK_SUBDEVICE_BACKEND_H*/

