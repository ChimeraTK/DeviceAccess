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
#include <ControlSystemAdapter/DevicePVManager.h>

#include "ApplicationException.h"
#include "VariableNetwork.h"
#include "Flags.h"

namespace ChimeraTK {

  class ApplicationModule;
  class AccessorBase;

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
      Application();

      /** Destructor */
      virtual ~Application() {}

      /** Set the process variable manager. This will be called by the control system adapter initialisation code. */
      void setPVManager(boost::shared_ptr<mtca4u::DevicePVManager> const &processVariableManager) {
        _processVariableManager = processVariableManager;
      }

      /** Obtain the process variable manager. */
      boost::shared_ptr<mtca4u::DevicePVManager> getPVManager() {
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

    protected:

      friend class ApplicationModule;

      template<typename UserType>
      friend class Accessor;

      /** To be implemented by the user: Instantiate all application modules and connect the variables to each other */
      virtual void initialise() = 0;

      /** Make the connections between accessors as requested in the initialise() function. */
      void makeConnections();

      /** Register a connection between two Accessors */
      void connectAccessors(AccessorBase &a, AccessorBase &b);

      /** Register an accessor to be published under the given name to the control system adapter */
      void publishAccessor(AccessorBase &a, const std::string& name);

      /** Register a connection of an accessor to a device register */
      void connectAccessorToDevice(AccessorBase &a, const std::string &deviceAlias,
          const std::string &registerName, UpdateMode mode, size_t numberOfElements, size_t elementOffsetInRegister);

      /** Perform the actual connection of an accessor to a device register */
      boost::shared_ptr<mtca4u::ProcessVariable> createDeviceAccessor(AccessorBase &a, const std::string &deviceAlias,
          const std::string &registerName, UpdateMode mode, size_t numberOfElements, size_t elementOffsetInRegister);

      /** Create a process variable with the PVManager, which is exported to the control system adapter */
      boost::shared_ptr<mtca4u::ProcessVariable> createProcessScalar(AccessorBase &a, const std::string &name);

      /** Create a local process variable which is not exported. The first element in the returned pair will be the
       *  sender, the second the receiver. */
      std::pair< boost::shared_ptr<mtca4u::ProcessVariable>, boost::shared_ptr<mtca4u::ProcessVariable> >
        createProcessScalar(AccessorBase &a);

      /** Register an application module with the application. Will be called automatically by all modules in their
       *  constructors. */
      void registerModule(ApplicationModule &module) {
        moduleList.push_back(&module);
      }

      /** List of application modules */
      std::list<ApplicationModule*> moduleList;

      /** List of variable networks */
      std::list<VariableNetwork> networkList;

      /** Find the network containing one of the given registers. If no network has been found, create an empty one */
      VariableNetwork& findOrCreateNetwork(AccessorBase *a, AccessorBase *b=nullptr);

      /** Map of accessor connections: the map key is the output accessor (providing the data) and the map target is
       *  a list of input accessors (consuming the data). */
//      std::map< AccessorBase*, std::list<AccessorBase*> > connectionMap;

      /** List of published accessors */
//      std::list< boost::shared_ptr<AccessorBase> > publicationList;

      /** Map of accessor-to-device connections: the map key is the accessor and the map target is the implementation
       *  used to access the device register */
//      std::map< AccessorBase*, boost::shared_ptr<mtca4u::ProcessVariable> > deviceAccessorMap;

      /** Pointer to the process variable manager used to create variables exported to the control system */
      boost::shared_ptr<mtca4u::DevicePVManager> _processVariableManager;

      /** Pointer to the only instance of the Application */
      static Application *instance;

      /** Mutex for thread-safety when setting the instance pointer */
      static std::mutex instance_mutex;

      /** Map of DeviceBackends used by this application. The map key is the alias name from the DMAP file */
      std::map<std::string, boost::shared_ptr<mtca4u::DeviceBackend>> deviceMap;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_APPLICATION_H */
