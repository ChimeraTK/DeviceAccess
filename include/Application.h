/*
 * Application.h
 *
 *  Created on: Jun 07, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_APPLICATION_H
#define CHIMERATK_APPLICATION_H

#include <mutex>

#include <mtca4u/DeviceBackend.h>
#include <ChimeraTK/ControlSystemAdapter/ApplicationBase.h>

#include "ApplicationException.h"
#include "VariableNetwork.h"
#include "Flags.h"
#include "ImplementationAdapter.h"

namespace ChimeraTK {

  class Module;
  class AccessorBase;
  class VariableNetwork;

  template<typename UserType>
  class Accessor;

  template<typename UserType>
  class DeviceAccessor;

  class Application : public ApplicationBase {

    public:

      Application(const std::string& name)
      : ApplicationBase(name) {}

      ~Application() {}

      /** This will remove the global pointer to the instance and allows creating another instance
       *  afterwards. This is mostly useful for writing tests, as it allows to run several applications sequentially
       *  in the same executable. Note that any ApplicationModules etc. owned by this Application are no longer
       *  valid after destroying the Application and must be destroyed as well (or at least no longer used). */
      void shutdown();
      
      /** Define the connections between process variables. Must be implemented by the application developer. */
      virtual void defineConnections() = 0;
      
      void initialise();

      void run();
      
      /** Check if all connections are valid. */
      void checkConnections();

      /** Instead of running the application, just initialise it and output the published variables to an XML file. */
      void generateXML();

      /** Output the connections requested in the initialise() function to std::cout. This may be done also before
       *  makeConnections() has been called. */
      void dumpConnections();

      /** Obtain instance of the application. Will throw an exception if called before the instance has been
       *  created by the control system adapter, or if the instance is not based on the Application class. */
      static Application& getInstance();

    protected:

      friend class Module;
      friend class VariableNetwork;
      friend class VariableNetworkNode;

      template<typename UserType>
      friend class Accessor;
      
      /** Register the connections to constants for previously unconnected nodes. */
      void processUnconnectedNodes();

      /** Make the connections between accessors as requested in the initialise() function. */
      void makeConnections();

      /** Make the connections for a single network */
      void makeConnectionsForNetwork(VariableNetwork &network);

      /** UserType-dependent part of makeConnectionsForNetwork() */
      template<typename UserType>
      void typedMakeConnection(VariableNetwork &network);

      /** Register a connection between two VariableNetworkNode */
      VariableNetwork& connect(VariableNetworkNode a, VariableNetworkNode b);

      /** Perform the actual connection of an accessor to a device register */
      template<typename UserType>
      boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> createDeviceVariable(const std::string &deviceAlias,
          const std::string &registerName, VariableDirection direction, UpdateMode mode);

      /** Create a process variable with the PVManager, which is exported to the control system adapter. nElements will
          be the array size of the created variable. */
      template<typename UserType>
      boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> createProcessVariable(VariableDirection direction,
          const std::string &name, size_t nElements);

      /** Create a local process variable which is not exported. The first element in the returned pair will be the
       *  sender, the second the receiver. nElements will be the array size of the created variable. */
      template<typename UserType>
      std::pair< boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>>, boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> >
            createApplicationVariable(size_t nElements);

      /** Register an application module with the application. Will be called automatically by all modules in their
       *  constructors. */
      void registerModule(Module &module) {
        moduleList.push_back(&module);
      }

      /** List of application modules */
      std::list<Module*> moduleList;

      /** List of ImplementationAdapters */
      std::list<boost::shared_ptr<ImplementationAdapterBase>> adapterList;

      /** List of variable networks */
      std::list<VariableNetwork> networkList;

      /** List of constant variable nodes */
      std::list<VariableNetworkNode> constantList;

      /** Create a new, empty network */
      VariableNetwork& createNetwork();

      /** Instance of VariableNetwork to indicate an invalid network */
      VariableNetwork invalidNetwork;

      /** Map of DeviceBackends used by this application. The map key is the alias name from the DMAP file */
      std::map<std::string, boost::shared_ptr<mtca4u::DeviceBackend>> deviceMap;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_APPLICATION_H */
