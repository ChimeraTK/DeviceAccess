#ifndef CHIMERA_TK_BACKEND_FACTORY_H
#define CHIMERA_TK_BACKEND_FACTORY_H

#ifdef _DEBUG
#include <iostream>
#endif

#include "ForwardDeclarations.h"
#include "DeviceInfoMap.h"
#include <map>
#include <mutex>
#include <boost/function.hpp>

/* For test purposes; if a dummies.dmap file is found in the folder from where the
 * program is being executed it would be used as dmap file. The default dmap file
 * would be DMAP_FILE_PATH.
 */
#define TEST_DMAP_FILE_PATH  "./dummies.dmap"   // FIXME remove
#define ENV_VAR_DMAP_FILE "DMAP_PATH_ENV"   // FIXME remove

namespace ChimeraTK {

  /** BackendFactory is a the factory class to create devices.
   * It is implemented as a Meyers' singleton.
   */
  class BackendFactory {
    public:
      /** This function sets the _DMapFilePath. This dmap file path is the
       *  second path where factory looks for dmap file. The first location
       *  where dmap file path is searched by library is set by enviroment
       *  variable.
       */
      void setDMapFilePath(std::string dMapFilePath);

      /** Returns the _DMapFilePath */
      std::string getDMapFilePath();

      /** This functions add new device using uri as a key. If a key already exist
       * it replaces it @todo FIXME don't replace. Check that if it is different (double
       * registration of of the same creator function should be possibe) and throw if not the same.
       * The version string has to match CHIMERATK_DEVICEACCESS_VERSION from DeviceAccessVersion.h,
       * otherwise a DeviceException is thrown. This prevents incompatible plugins to be loaded. */
      void registerBackendType(std::string backendType,
        boost::shared_ptr<DeviceBackend> (*creatorFunction)(std::string host, std::string instance,
                                                            std::map<std::string,std::string> parameters),
        std::string version);

      /** Old signature of BackendFactory::registerBackendType(), for compatibility only. Please use only the new form
       *  which allows to pass key-equal-value pairs for the parameters. DO NOT call this function in addition to the
       *  new signature! */
      void registerBackendType(std::string interface, std::string protocol,
        boost::shared_ptr<DeviceBackend> (*creatorFunction)(std::string host, std::string instance,
                                                            std::list<std::string> parameters, std::string),
        std::string version);

      /** Create a new backend and return the instance as a shared pointer.
       *  The input argument can either be an alias name from a dmap file, or
       *  an sdm::/ URI.
       */
      boost::shared_ptr<DeviceBackend> createBackend(std::string aliasOrUri);

      /**Static function to get an instance of factory */
      static BackendFactory& getInstance();

      /**Load a shared library (.so file) with a Backend at run time.
       */
      void loadPluginLibrary(std::string soFile);

    protected:
      std::string _dMapFile; ///< The dmap file set at run time

      /** Holds device type and function pointer to the createInstance function of plugin */
      std::map< std::string, boost::function< boost::shared_ptr<DeviceBackend> (std::string host, std::string instance, std::map<std::string,std::string> parameters) > > creatorMap;

      /** Compatibility creatorMap for old-style backends which just take a plain list of parameters */
      std::map< std::pair<std::string, std::string>, boost::function< boost::shared_ptr<DeviceBackend> (std::string host, std::string instance, std::list<std::string>parameters, std::string mapFileName) > > creatorMap_compat;

      /** Look for the alias and if found return a uri */
      std::string aliasLookUp(std::string aliasName, std::string dmapFilePath);

      /** Internal function to return a DeviceBackend */
      boost::shared_ptr<DeviceBackend> createBackendInternal(const DeviceInfoMap::DeviceInfo &deviceInfo);

      /** A map of all created backends. If the same URI is located again, the existing backend is returned. */
      std::map< std::string, boost::weak_ptr<DeviceBackend> > _existingBackends;

      /** A mutex to protect backend creation and returning to keep the maps consistent.*/
      std::mutex _mutex;

      // A function which has the signature of a creator function, plus one string 'exception_what'.
      // If a plugins fails to register, this function is bound to an error string and stored in the creatorMap. It later it is tried to open the backend, an exception with this
      // error message is thrown.
      static boost::shared_ptr<DeviceBackend> failedRegistrationThrowerFunction(std::string host, std::string instance, std::list<std::string>parameters, std::string mapFileName, std::string exception_what);

      /**Load all shared libraries specified in the dmap file.
       */
      void loadAllPluginsFromDMapFile();

    private:
      // These functions are not to be inherited. As also the constructor is private, you cannot
      // derrive from this class.
      /** Singleton: The constructor can only be called from the getInstance() function. */
      BackendFactory();

      BackendFactory(BackendFactory const&);     /**< To avoid making copies */
      BackendFactory(BackendFactory const&&);     /**< To avoid making copies */
      void operator=(BackendFactory const&); /**< To avoid assignment */
  };

}// namespace ChimeraTK

#endif /* CHIMERA_TK_BACKEND_FACTORY_H */
