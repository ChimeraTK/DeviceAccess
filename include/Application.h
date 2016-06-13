/*
 * Application.h
 *
 *  Created on: Jun 07, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_APPLICATION_H
#define CHIMERATK_APPLICATION_H

#include <mutex>

#include <ControlSystemAdapter/DevicePVManager.h>

namespace ChimeraTK {

  /** Enum to define directions of variables. The direction is always defined from the point-of-view of the
   *  definer, i.e. the application module owning the instance of the accessor in this context. */
  enum class VariableDirection {
    input, output
  };

  /** Enum to define the update mode of variables. */
  enum class UpdateMode {
    pull, push
  };

  class ApplicationModule;
  class AccessorBase;

  template<typename UserType>
  class Accessor;

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

      /** Obtain instance of the application. Will throw an exception if called before the instance has been
       *  created by the control system adapter. */
      static Application& getInstance() {
        // @todo TODO Throw the exception if instance==nullptr !!!
        return *instance;
      }

      /** Register a connection between two Accessors */
      void connectAccessors(AccessorBase &a, AccessorBase &b);

      /** Register an accessor to be published under the given name to the control system adapter */
      template<typename UserType>
      void publishAccessor(Accessor<UserType> &a, const std::string& name);

    protected:

      /** To be implemented by the user: Instantiate all application modules and connect the variables to each other */
      virtual void initialise() = 0;

      /** Make the connections between accessors as requested in the initialise() function. */
      void makeConnections();

      friend class ApplicationModule;

      /** Register an application module with the application. Will be called automatically by all modules in their
       *  constructors. */
      void registerModule(ApplicationModule &module) {
        moduleList.push_back(&module);
      }

      /** List of application modules */
      std::list<ApplicationModule*> moduleList;

      template<typename UserType>
      friend class Accessor;

      /** Map of accessor connections: the map key is the output accessor (providing the data) and the map target is
       *  a list of input accessors (consuming the data). */
      std::map< AccessorBase*, std::list<AccessorBase*> > connectionMap;

      /** List of published accessors */
      std::list< boost::shared_ptr<AccessorBase> > publicationList;

      /** Pointer to the process variable manager used to create variables exported to the control system */
      boost::shared_ptr<mtca4u::DevicePVManager> _processVariableManager;

      /** Pointer to the only instance of the Application */
      static Application *instance;

      /** Mutex for thread-safety when setting the instance pointer */
      static std::mutex instance_mutex;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_APPLICATION_H */
