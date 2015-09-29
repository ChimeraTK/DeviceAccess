/*
 * BufferingRegisterAccessor.h
 *
 *  Created on: Sep 28, 2015
 *      Author: Martin Hierholzer <martin.hierholzer@desy.de>
 */

#ifndef SOURCE_DIRECTORY__DEVICE_INCLUDE_BUFFERINGREGISTERACCESSOR_H_
#define SOURCE_DIRECTORY__DEVICE_INCLUDE_BUFFERINGREGISTERACCESSOR_H_

#include <vector>
#include "Device.h"
#include "FixedPointConverter.h"

namespace mtca4u {

  /*********************************************************************************************************************/
  /** Accessor class to read and write registers transparently by using the accessor object like a variable of the
   *  type UserType. Conversion to and from the UserType will be handled by the FixedPointConverter matching the
   *  register description in the map. Obtain the accessor using the Device::getBufferingRegisterAccessor() function.
   *
   *  Note: Transfers between the device and the internal buffer need to be triggered using the read() and write()
   *  functions before reading from resp. after writing to the buffer using the operators.
   */
  template<typename T>
  class BufferingRegisterAccessor {
    public:

      /** Constructer. @attention Do not normally use directly.
       *  Users should call Device::getBufferingRegisterAccessor() to obtain an instance instead.
       */
      BufferingRegisterAccessor(boost::shared_ptr<DeviceBackend> dev, RegisterInfoMap::RegisterInfo &registerInfo)
          : _registerInfo(registerInfo),
            _dev(dev) {
        fixedPointConverter = FixedPointConverter(registerInfo.reg_width, registerInfo.reg_frac_bits,
            registerInfo.reg_signed);
        rawBuffer.resize(registerInfo.reg_elem_nr);
        cookedBuffer.resize(registerInfo.reg_elem_nr);
      }

      /** Placeholder constructer, to allow late initialisation of the accessor, e.g. in the open function.
       *  @attention Accessors created with this constructors will be dysfunctional!
       */
      BufferingRegisterAccessor() {}

      /** Read the data from the device, convert it and store in buffer.
       */
      void read() {
        _dev->read(_registerInfo.reg_bar, _registerInfo.reg_address, rawBuffer.data(),
            getNumberOfElements() * sizeof(uint32_t));
        for(unsigned int i = 0; i < getNumberOfElements(); i++) {
          cookedBuffer[i] = fixedPointConverter.template toCooked<T>(rawBuffer[i]);
        }
      }

      /** Convert data from the buffer and write to device.
       */
      void write() {
        for(unsigned int i = 0; i < getNumberOfElements(); i++) {
          rawBuffer[i] = fixedPointConverter.toRaw(cookedBuffer[i]);
        }
        _dev->write(_registerInfo.reg_bar, _registerInfo.reg_address, rawBuffer.data(),
            getNumberOfElements() * sizeof(uint32_t));
      }

      /** Get or set buffer content by [] operator.
       *  @attention No bounds checking is performed, use getNumberOfElements() to obtain the number of elements in
       *  the register.
       */
      inline T& operator[](unsigned int index) {
        return cookedBuffer[index];
      }

      /** Return number of elements
       */
      inline unsigned int getNumberOfElements() {
        return cookedBuffer.size();
      }

      /** Access the 0th element on l.h.s. with = operator
       */
      inline T& operator=(T rhs) {
        return cookedBuffer[0] = rhs;
      }

      /** Access the 0th element on r.h.s. via type conversion
       */
      inline operator T() {
        return cookedBuffer[0];
      }

      /** Factory function as called by Device::getBufferingRegisterAccessor() and Device::getCustomAccessor()
       */
      static BufferingRegisterAccessor<T> createInstance(
          std::string const &dataRegionName, std::string const &module, boost::shared_ptr<DeviceBackend> const &backend,
          boost::shared_ptr<RegisterInfoMap> const &registerMapping) {
        RegisterInfoMap::RegisterInfo registerInfo;
        registerMapping->getRegisterInfo(dataRegionName, registerInfo, module);
        return BufferingRegisterAccessor<T>(backend, registerInfo);
      }

    protected:

      /// register information from the map
      RegisterInfoMap::RegisterInfo _registerInfo;

      /// pointer to VirtualDevice
      boost::shared_ptr<DeviceBackend> _dev;

      /// fixed point converter
      FixedPointConverter fixedPointConverter;

      /// vector of unconverted data elements
      std::vector<int32_t> rawBuffer;

      /// vector of converted data elements
      std::vector<T> cookedBuffer;

  };

}    // namespace mtca4u

#endif /* SOURCE_DIRECTORY__DEVICE_INCLUDE_BUFFERINGREGISTERACCESSOR_H_ */
