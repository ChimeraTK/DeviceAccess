/*
 * LNMBackendRegisterInfo.h
 *
 *  Created on: Mar 24, 2016
 *      Author: mhier
 */

#ifndef CHIMERA_TK_LNM_BACKEND_REGISTER_INFO_H
#define CHIMERA_TK_LNM_BACKEND_REGISTER_INFO_H

#include <mutex>

#include <boost/shared_ptr.hpp>

#include "ForwardDeclarations.h"
#include "RegisterInfo.h"

namespace ChimeraTK {

  namespace LNMBackend {
    class AccessorPluginBase;
  }

  /** RegisterInfo structure for the LogicalNameMappingBackend */
  class LNMBackendRegisterInfo : public RegisterInfo {
   public:
    /** Potential target types */
    enum TargetType { INVALID, REGISTER, CHANNEL, BIT, CONSTANT, VARIABLE };

    /** constuctor: initialise values */
    LNMBackendRegisterInfo() : targetType(TargetType::INVALID), supportedFlags({}) {}

    RegisterPath getRegisterName() const override { return name; }

    unsigned int getNumberOfElements() const override { return length; }

    unsigned int getNumberOfDimensions() const override { return nDimensions; }

    unsigned int getNumberOfChannels() const override { return nChannels; }

    const DataDescriptor& getDataDescriptor() const override { return _dataDescriptor; }

    bool isReadable() const override { return readable; }

    bool isWriteable() const override { return writeable; }

    AccessModeFlags getSupportedAccessModes() const override { return supportedFlags; }

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

    /** The channel of the target 2D register (if TargetType::CHANNEL) */
    unsigned int channel;

    /** The bit of the target register (if TargetType::BIT) */
    unsigned int bit;

    /** The number of dimensions of the logical register */
    unsigned int nDimensions;

    /** The number of channels of the logical register */
    unsigned int nChannels;

    /** Data type of CONSTANT or VARIABLE type. */
    DataType valueType;

    /** Hold values of CONSTANT or VARIABLE types in a type-dependent table. Only the entry matching the valueType
     *  is actually valid.
     *  Note: We cannot directly put the std::vector in a TemplateUserTypeMap, since it has more than one template
     *  arguments (with defaults). */
    template<typename T>
    using myVector = std::vector<T>;
    TemplateUserTypeMap<myVector> valueTable;

    /** Mutex one needs to hold while accessing valueTable */
    std::mutex valueTable_mutex;

    /** Flag if the register is readable. Might be derived from the target
     * register */
    bool readable;

    /** Flag if the register is writeable. Might be derived from the target
     * register */
    bool writeable;

    /** Supported AccessMode flags. Might be derived from the target register */
    AccessModeFlags supportedFlags;

    /** List of accessor plugins enabled for this register */
    std::vector<boost::shared_ptr<LNMBackend::AccessorPluginBase>> plugins;

    DataDescriptor _dataDescriptor;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERA_TK_LNM_BACKEND_REGISTER_INFO_H */
