#ifndef _MTCA4U_DEVICE_FACTORY_H__
#define _MTCA4U_DEVICE_FACTORY_H__


#include "DummyBackend.h"
#include "PcieBackend.h"
#include <boost/tuple/tuple.hpp>
#include <boost/filesystem.hpp>
#include "DMapFile.h"
//#define _DEBUG
#ifdef _DEBUG
#include <iostream>
#endif

/* For test purposes; if a dummies.dmap file is found in the folder from where the
 * program is being executed it would be used as dmap file. The default dmap file
 * would be DMAP_FILE_PATH.
 */
#define DMAP_FILE_PATH  "/usr/local/etc/mtca4u/BackendFactory.dmap"
#define TEST_DMAP_FILE_PATH  "/dummies.dmap"
namespace mtca4u {

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
private:
	/** Add known device */
	BackendFactory() {
		registerDeviceType("pci","",&PcieBackend::createInstance);
		registerDeviceType("pci","pcie",&PcieBackend::createInstance);
		registerDeviceType("dummy","",&DummyBackend::createInstance);
	};

	boost::tuple<boost::shared_ptr<DeviceBackend>, DMapFile::dRegisterInfo> parseDMap(std::string devName);

	//BackendFactory(BackendFactory const&);     /** To avoid making copies */

	void operator=(BackendFactory const&); /** To avoid making copies */

	/** Holds  device type and function pointer to the createInstance function of
	 * plugin*/

	std::map< std::pair<std::string, std::string>, boost::shared_ptr<DeviceBackend> (*)(std::string host, std::string instance, std::list<std::string>parameters) > creatorMap;


public:
	/** This functions add new device using uri as a key. If a key already exist
	 * it replaces it*/
	void registerDeviceType(std::string interface, std::string protocol,
			boost::shared_ptr<DeviceBackend> (*creatorFunction)(std::string host, std::string instance, std::list<std::string>parameters));

	/** Create a new device by calling the constructor and returning a pointer to
	 * the
	 * device */

	boost::shared_ptr<DeviceBackend> createDevice(std::string aliasName);

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
