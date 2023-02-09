// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "DeviceAccessVersion.h"
#include "DeviceInfoMap.h"
#include "ForwardDeclarations.h"

#include <boost/function.hpp>

#include <map>
#include <mutex>

/* For test purposes; if a dummies.dmap file is found in the folder from where
 * the program is being executed it would be used as dmap file. The default dmap
 * file would be DMAP_FILE_PATH.
 */
#define TEST_DMAP_FILE_PATH "./dummies.dmap" // FIXME remove
#define ENV_VAR_DMAP_FILE "DMAP_PATH_ENV"    // FIXME remove

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

    /**
     *  Register a backend by the name backendType with the given creatorFunction.
     * If a backend by the given name is already registered, a
     * ChimeraTK::logic_error is thrown.
     *
     *  The optional parameter sdmParameterNames specifies a list of key names,
     * which is used when a device is created using a SDM URI. The parameters from
     * the (unkeyed) parameter list of the SDM are put into in order the parameter
     * map by the keys specified in sdmParameterNames. If sdmParameterNames is
     * left empty, the device cannot be created through a SDM URI when parameters
     * need to be specified.
     *
     *  The last argument deviceAccessVersion must always be set to
     * CHIMERATK_DEVICEACCESS_VERSION as defined in DeviceAccessVersion.h. This is
     * automatically the case if the argument is omitted, so it should never be
     *  specified.
     */
    void registerBackendType(const std::string& backendType,
        boost::shared_ptr<DeviceBackend> (*creatorFunction)(
            std::string address, std::map<std::string, std::string> parameters),
        const std::vector<std::string>& sdmParameterNames = {},
        const std::string& deviceAccessVersion = CHIMERATK_DEVICEACCESS_VERSION);

    /** Old signature of BackendFactory::registerBackendType(), for compatibility
     * only. Please use only the new form which allows to pass key-equal-value
     * pairs for the parameters. DO NOT call this function in addition to the
     *  new signature! */
    [[deprecated]] void registerBackendType(const std::string& interface, const std::string& protocol,
        boost::shared_ptr<DeviceBackend> (*creatorFunction)(
            std::string host, std::string instance, std::list<std::string> parameters, std::string mapFileName),
        const std::string& version);

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

    /** Holds device type and function pointer to the createInstance function of
     * plugin */
    std::map<std::string,
        boost::function<boost::shared_ptr<DeviceBackend>(
            std::string address, std::map<std::string, std::string> parameters)>>
        creatorMap;

    /** Compatibility creatorMap for old-style backends which just take a plain
     * list of parameters */
    std::map<std::pair<std::string, std::string>,
        boost::function<boost::shared_ptr<DeviceBackend>(
            std::string host, std::string instance, std::list<std::string> parameters, std::string mapFileName)>>
        creatorMap_compat;

    /** Look for the alias and if found return a uri */
    std::string aliasLookUp(std::string aliasName, std::string dmapFilePath);

    /** Internal function to return a DeviceBackend */
    boost::shared_ptr<DeviceBackend> createBackendInternal(const DeviceInfoMap::DeviceInfo& deviceInfo);

    /** A map of all created backends. If the same URI is located again, the
     * existing backend is returned. */
    std::map<std::string, boost::weak_ptr<DeviceBackend>> _existingBackends;

    /** A mutex to protect backend creation and returning to keep the maps
     * consistent.*/
    std::mutex _mutex;

    // A function which has the signature of a creator function, plus one string
    // 'exception_what'. If a plugins fails to register, this function is bound to
    // an error string and stored in the creatorMap. It later it is tried to open
    // the backend, an exception with this error message is thrown.
    static boost::shared_ptr<DeviceBackend> failedRegistrationThrowerFunction(std::string host, std::string instance,
        std::list<std::string> parameters, std::string mapFileName, std::string exception_what);

    /**Load all shared libraries specified in the dmap file.
     */
    void loadAllPluginsFromDMapFile();

    /** Flag whether the function registerBackendType() was called. This is used
     * to determine if a loaded plugin registered any backends. */
    bool called_registerBackendType{false};

   private:
    // These functions are not to be inherited. As also the constructor is
    // private, you cannot derrive from this class.
    /** Singleton: The constructor can only be called from the getInstance()
     * function. */
    BackendFactory();

    BackendFactory(BackendFactory const&);           /**< To avoid making copies */
    BackendFactory(BackendFactory const&&) noexcept; /**< To avoid making copies */
    void operator=(BackendFactory const&);           /**< To avoid assignment */
  };

} // namespace ChimeraTK
