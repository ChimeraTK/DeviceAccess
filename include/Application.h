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
#include <ChimeraTK/ControlSystemAdapter/DevicePVManager.h>

#include "ApplicationException.h"
#include "VariableNetwork.h"
#include "Flags.h"
#include "ImplementationAdapter.h"

namespace ChimeraTK {

  class Module;
  class AccessorBase;
  class InvalidAccessor;
  class VariableNetwork;

  template<typename UserType>
  class Accessor;

  template<typename UserType>
  class DeviceAccessor;

  class Application {

    public:

      /** Constructor: the first instance will be created explicitly by the control system adapter code. Any second
       *  instance is not allowed, thus calling the constructor multiple times will throw an exception.
       *  Design note: We are not using a true singleton pattern, since Application is an abstract base class. The
       *  actual instance is created as a static variable.
       *  The application developer should derive his application from this class and implement the initialise()
       *  function only. */
      Application(const std::string& name);

      /** The implementation of Application must call Application::shutdown() in its destructor. This destructor just
       *  checks if Application::shutdown() was properly called and throws an exception otherwise. */
      virtual ~Application();

      /** This will remove the global pointer to the instance and allows creating another instance
       *  afterwards. This is mostly useful for writing tests, as it allows to run several applications sequentially
       *  in the same executable. Note that any ApplicationModules etc. owned by this Application are no longer
       *  valid after destroying the Application and must be destroyed as well (or at least no longer used). */
      void shutdown();

      /** Set the process variable manager. This will be called by the control system adapter initialisation code. */
      void setPVManager(boost::shared_ptr<ChimeraTK::DevicePVManager> const &processVariableManager) {
        _processVariableManager = processVariableManager;
      }

      /** Obtain the process variable manager. */
      boost::shared_ptr<ChimeraTK::DevicePVManager> getPVManager() {
        return _processVariableManager;
      }

      /** Initialise and run the application */
      void run();

      /** Instead of running the application, just initialise it and output the published variables to an XML file. */
      void generateXML();

      /** Obtain instance of the application. Will throw an exception if called before the instance has been
       *  created by the control system adapter. */
      static Application& getInstance() {
        // @todo TODO Throw the exception if instance==nullptr !!!
        return *instance;
      }

      /** Output the connections requested in the initialise() function to std::cout. This may be done also before
       *  makeConnections() has been called. */
      void dumpConnections();

      /** Return the name of the application */
      const std::string& getName() {return applicationName;}

    protected:

      friend class Module;
      friend class VariableNetwork;
      friend class VariableNetworkNode;

      template<typename UserType>
      friend class Accessor;

      /** To be implemented by the user: Instantiate all application modules and connect the variables to each other */
      virtual void initialise() = 0;

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
      boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> createDeviceAccessor(const std::string &deviceAlias,
          const std::string &registerName, VariableDirection direction, UpdateMode mode);

      /** Create a process variable with the PVManager, which is exported to the control system adapter */
      template<typename UserType>
      boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> createProcessScalar(VariableDirection direction,
          const std::string &name);

      /** Create a local process variable which is not exported. The first element in the returned pair will be the
       *  sender, the second the receiver. */
      template<typename UserType>
      std::pair< boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>>, boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> >
        createProcessScalar();

      /** Register an application module with the application. Will be called automatically by all modules in their
       *  constructors. */
      void registerModule(Module &module) {
        moduleList.push_back(&module);
      }

      /** The name of the application */
      std::string applicationName;

      /** List of application modules */
      std::list<Module*> moduleList;

      /** List of ImplementationAdapters */
      std::list<boost::shared_ptr<ImplementationAdapterBase>> adapterList;

      /** List of variable networks */
      std::list<VariableNetwork> networkList;

      /** Create a new, empty network */
      VariableNetwork& createNetwork();

      /** Instance of VariableNetwork to indicate an invalid network */
      VariableNetwork invalidNetwork;

      /** Pointer to the process variable manager used to create variables exported to the control system */
      boost::shared_ptr<ChimeraTK::DevicePVManager> _processVariableManager;

      /** Pointer to the only instance of the Application */
      static Application *instance;

      /** Mutex for thread-safety when setting the instance pointer */
      static std::mutex instance_mutex;

      /** Map of DeviceBackends used by this application. The map key is the alias name from the DMAP file */
      std::map<std::string, boost::shared_ptr<mtca4u::DeviceBackend>> deviceMap;

      /** Flag if shutdown() has been called. */
      bool hasBeenShutdown{false};

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_APPLICATION_H */
