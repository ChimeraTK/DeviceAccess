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

      /** The integer value of a INT_CONSTANT or INT_VARIABLE */
      std::vector<int> value_int;

    protected:

      friend class LogicalNameMapParser;
      friend class LogicalNameMappingBackend;

      DataDescriptor _dataDescriptor;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERA_TK_LNM_BACKEND_REGISTER_INFO_H */
