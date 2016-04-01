/*
 * LNMBackendRegisterInfo.h
 *
 *  Created on: Mar 24, 2016
 *      Author: mhier
 */

#ifndef MTCA4U_LNM_BACKEND_REGISTER_INFO_H
#define MTCA4U_LNM_BACKEND_REGISTER_INFO_H

#include "RegisterInfo.h"
#include "DynamicValue.h"
#include "ForwardDeclarations.h"

namespace mtca4u {

  /** RegisterInfo structure for the LogicalNameMappingBackend */
  class LNMBackendRegisterInfo : public RegisterInfo {
    public:

      /** Potential target types */
      enum TargetType { INVALID, REGISTER, CHANNEL, INT_CONSTANT, INT_VARIABLE };

      /** constuctor: initialise values */
      LNMBackendRegisterInfo()
      : targetType(TargetType::INVALID)
      {}

      virtual RegisterPath getRegisterName() const {
        return name;
      }

      virtual unsigned int getNumberOfElements() const {
        return length;
      }

      virtual unsigned int getNumberOfDimensions() const {
        return nDimensions;
      }

      virtual unsigned int getNumberOfChannels() const {
        return nChannels;
      }

      /** Name of the registrer */
      RegisterPath name;

      /** Type of the target */
      TargetType targetType;

      /** The target device alias */
      DynamicValue<std::string> deviceName;

      /** The target register name */
      DynamicValue<std::string> registerName;

      /** The first index in the range */
      DynamicValue<unsigned int> firstIndex;

      /** The length of the range (i.e. number of indices) */
      DynamicValue<unsigned int> length;

      /** The channel of the target 2D register */
      DynamicValue<unsigned int> channel;

      /** The number of dimensions of the logical register */
      DynamicValue<unsigned int> nDimensions;

      /** The number of channels of the logical register */
      DynamicValue<unsigned int> nChannels;

      /** The constant integer value */
      DynamicValue<int> value;

      /** test if deviceName is set (depending on the targetType) */
      bool hasDeviceName() const {
        return targetType != TargetType::INT_CONSTANT && targetType != TargetType::INT_VARIABLE;
      }

      /** test if registerName is set (depending on the targetType) */
      bool hasRegisterName() const {
        return targetType != TargetType::INT_CONSTANT && targetType != TargetType::INT_VARIABLE;
      }

      /** test if firstIndex is set (depending on the targetType) */
      bool hasFirstIndex() const {
        return targetType == TargetType::REGISTER;
      }

      /** test if length is set (depending on the targetType) */
      bool hasLength() const {
        return targetType == TargetType::REGISTER;
      }

      /** test if channel is set (depending on the targetType) */
      bool hasChannel() const {
        return targetType == TargetType::CHANNEL;
      }

      /** test if value is set (depending on the targetType) */
      bool hasValue() const {
        return targetType == TargetType::INT_CONSTANT || targetType == TargetType::INT_VARIABLE;
      }

      /** create the internal register accessors */
      void createInternalAccessors(boost::shared_ptr<DeviceBackend> &backend) {
        deviceName.createInternalAccessors(backend);
        registerName.createInternalAccessors(backend);
        firstIndex.createInternalAccessors(backend);
        length.createInternalAccessors(backend);
        channel.createInternalAccessors(backend);
        value.createInternalAccessors(backend);
      }

    protected:

      friend class LogicalNameMapParser;
  };

} /* namespace mtca4u */

#endif /* MTCA4U_LNM_BACKEND_REGISTER_INFO_H */
