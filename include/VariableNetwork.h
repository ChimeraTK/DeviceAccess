/*
 * VariableNetwork.h
 *
 *  Created on: Jun 14, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_VARIABLE_NETWORK_H
#define CHIMERATK_VARIABLE_NETWORK_H

#include <list>
#include <string>
#include <typeinfo>
#include <boost/mpl/for_each.hpp>

#include "Flags.h"

namespace ChimeraTK {

  class AccessorBase;

  /** This class describes a network of variables all connected to each other. */
  class VariableNetwork {

    public:

      /** Define accessor types */
      enum class AccessorType {
          Device, ControlSystem, Application, Undefined
      };

      /** Struct with DeviceAccess register information */
      struct DeviceRegisterInfo {
          std::string deviceAlias;
          std::string registerName;
          UpdateMode mode;
      };

      /** Add an accessor to the network. */
      void addAccessor(AccessorBase &a);

      /** Add control-system-to-device publication. The given accessor will be used to derive the requred value type.
       *  The name will be the name of the process variable visible in the control system adapter. */
      void addCS2DevPublication(AccessorBase &a, const std::string& name);

      /** Add device-to-control-system publication. */
      void addDev2CSPublication(const std::string& name);

      /** Add */
      void addInputDeviceRegister(const std::string &deviceAlias, const std::string &registerName,
          UpdateMode mode, size_t numberOfElements, size_t elementOffsetInRegister);

      void addOutputDeviceRegister(const std::string &deviceAlias, const std::string &registerName,
          UpdateMode mode, size_t numberOfElements, size_t elementOffsetInRegister);

      /** Check if the network already has an output-type accessor connected to it (note: an output-type accessor
       *  will *povide* values to this network, so from the network's point-of-view it will be the input. */
      bool hasOutputAccessor() const;

      /** Count the number of input accessors in the network */
      size_t countInputAccessors() const;

      /** Check if either of the given accessors is part of this network. If the second argument is omitted, only
       *  the first accessor will be checked. */
      bool hasAccessor(AccessorBase *a, AccessorBase *b=nullptr) const;

      /** Obtain the type info of the UserType. If the network type has not yet been determined (i.e. if no output
       *  accessor has been assigned yet), the typeid of void will be returned. */
      const std::type_info& getValueType() const {
        return *valueType;
      }

      /** Check if the network is connected to an input accessor of the given type */
      bool hasInputAccessorType(AccessorType type) const;

      /** Return the type of the output accessor (i.e. the accessor providing values to the network) */
      AccessorType getOutputAccessorType() const;

      /** Return list of Application accessors */
      const std::list<AccessorBase*>& getInputAccessorList() const { return inputAccessorList; }

      AccessorBase* getOutputAccessor() const { return outputAccessor; }

      const std::string& getOutputPublicationName() const { return publicCS2DevName; }

      const std::list<std::string>& getInputPublicationNames() const { return publicDev2CSNames; }

      const std::list<DeviceRegisterInfo>& getDeviceInputRegisterInfos() const { return deviceWriteRegisterInfos; }

      const DeviceRegisterInfo& getDeviceOutputRegisterInfo() const { return deviceReadRegisterInfo; }

      /** Dump the network structure to std::cout */
      void dump() const;

    protected:

      /** List of input-type accessors in the network. These accessors are usually provided by an ApplicationModule. */
      std::list<AccessorBase*> inputAccessorList;

      /** Output accessor for this network. */
      AccessorBase* outputAccessor{nullptr};

      /** Name of the variable if it is published to the control system in the control-system-to-device direction.
       *  If empty, no publication in this direction is requested. */
      std::string publicCS2DevName{""};

      /** List of names under which the variable is published to the control system in the device-to-control-system
       *  direction. */
      std::list<std::string> publicDev2CSNames;

      /** List of write registers to which this network is connected */
      std::list<DeviceRegisterInfo> deviceWriteRegisterInfos;

      /** Read register to which this network is connected. An empty deviceAlias indicates no read register has been
       *  connected. */
      DeviceRegisterInfo deviceReadRegisterInfo;

      /** The network value type id. Since in C++, std::type_info is non-copyable and typeid() returns a reference to
       *  an object with static storage duration, we have to (and can safely) store a pointer here. */
      const std::type_info* valueType{&typeid(void)};

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_VARIABLE_NETWORK_H */
