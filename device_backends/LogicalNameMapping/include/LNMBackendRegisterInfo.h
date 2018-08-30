/*
 * LNMBackendRegisterInfo.h
 *
 *  Created on: Mar 24, 2016
 *      Author: mhier
 */

#ifndef CHIMERA_TK_LNM_BACKEND_REGISTER_INFO_H
#define CHIMERA_TK_LNM_BACKEND_REGISTER_INFO_H

#include "RegisterInfo.h"
#include "ForwardDeclarations.h"

namespace ChimeraTK {

  /** RegisterInfo structure for the LogicalNameMappingBackend */
  class LNMBackendRegisterInfo : public RegisterInfo {
    public:

      /** Potential target types */
      enum TargetType { INVALID, REGISTER, CHANNEL, INT_CONSTANT, INT_VARIABLE };

      /** constuctor: initialise values */
      LNMBackendRegisterInfo()
      : targetType(TargetType::INVALID)
      {}

      RegisterPath getRegisterName() const override {
        return name;
      }

      unsigned int getNumberOfElements() const override {
        return length;
      }

      unsigned int getNumberOfDimensions() const override {
        return nDimensions;
      }

      unsigned int getNumberOfChannels() const override {
        return nChannels;
      }

      const DataDescriptor& getDataDescriptor() const override {
        return _dataDescriptor;
      }

      /** Name of the registrer */
      RegisterPath name;

      /** Type of the target */
      TargetType targetType;

      /** The target device alias */
      std::string deviceName;

      /** The target register name */
      std::string registerName;

      /** The first index in the range */
      unsigned int firstIndex;

      /** The length of the range (i.e. number of indices) */
      unsigned int length;

      /** The channel of the target 2D register */
      unsigned int channel;

      /** The number of dimensions of the logical register */
      unsigned int nDimensions;

      /** The number of channels of the logical register */
      unsigned int nChannels;

      /** The constant integer value */
      int value;

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

    protected:

      friend class LogicalNameMapParser;
      friend class LogicalNameMappingBackend;

      DataDescriptor _dataDescriptor;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERA_TK_LNM_BACKEND_REGISTER_INFO_H */
