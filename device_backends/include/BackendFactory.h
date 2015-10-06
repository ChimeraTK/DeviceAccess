#ifndef _MTCA4U_DEVICE_FACTORY_H__
#define _MTCA4U_DEVICE_FACTORY_H__


#include "DummyBackend.h"
#include "PcieBackend.h"
#include <boost/tuple/tuple.hpp>
#include <boost/filesystem.hpp>
#include "DMapFile.h"
#ifdef _DEBUG
#include <iostream>
#endif

/* For test purposes; if a dummies.dmap file is found in the folder from where the
 * program is being executed it would be used as dmap file. The default dmap file
 * would be DMAP_FILE_PATH.
 */
#define TEST_DMAP_FILE_PATH  "/dummies.dmap"
#define ENV_VAR_DMAP_FILE "DMAP_PATH_ENV"
namespace mtca4u {

/** A class to provide exception for BackendFactory.
 *
 */
class BackendFactoryException : public Exception {
public:
  enum {UNKNOWN_ALIAS,UNREGISTERED_DEVICE,AMBIGUOUS_MAP_FILE_ENTRY};
  BackendFactoryException(const std::string &message, unsigned int exceptionID)
  : Exception( message, exceptionID ){}
};

/** BackendFactory is a the factory class to create devices.
 * It is implemented as a Meyers' singleton.
 */
class BackendFactory {
public:
  std::string _dMapFilePath;
private:
  /** Add known device */
  BackendFactory() {
    _dMapFilePath = boost::filesystem::initial_path().string() + (std::string)TEST_DMAP_FILE_PATH;
    registerBackendType("pci","",&PcieBackend::createInstance);
    registerBackendType("pci","pcie",&PcieBackend::createInstance);
    registerBackendType("dummy","",&DummyBackend::createInstance);
  };

  //BackendFactory(BackendFactory const&);     /** To avoid making copies */

  void operator=(BackendFactory const&); /** To avoid making copies */

  /** Holds  device type and function pointer to the createInstance function of
   * plugin*/

  std::map< std::pair<std::string, std::string>, boost::shared_ptr<DeviceBackend> (*)(std::string host, std::string instance, std::list<std::string>parameters) > creatorMap;

  /** Look for the alias and if found return a uri */
  std::string aliasLookUp(std::string aliasName, std::string dmapFilePath);

  /** Internal function to return a DeviceBackend */
  boost::shared_ptr<DeviceBackend> createBackendInternal(std::string uri);

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
   * it replaces it*/
  void registerBackendType(std::string interface, std::string protocol,
      boost::shared_ptr<DeviceBackend> (*creatorFunction)(std::string host, std::string instance, std::list<std::string>parameters));

  /** Create a new device by calling the constructor and returning a pointer to
   * the
   * device */

  boost::shared_ptr<DeviceBackend> createBackend(std::string aliasName);

  /**Static function to get an instance of factory */
  static BackendFactory& getInstance() {
#ifdef _DEBUG
    std::cout << "getInstance" << std::endl << std::flush;
#endif
    static BackendFactory factoryInstance; /** Thread safe in C++11*/
    return factoryInstance;
  }
};

} // namespace mtca4u

#endif /* _MTCA4U_DEVICE_FACTORY_H__ */
