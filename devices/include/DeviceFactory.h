#ifndef _MTCA4U_DEVICE_FACTORY_H__
#define _MTCA4U_DEVICE_FACTORY_H__


#include "FakeDevice.h"
#include "BaseDevice.h"
#include "DummyDevice.h"
#include "MappedDevice.h"
#include "PcieDevice.h"
#include <boost/tuple/tuple.hpp>
#include <boost/filesystem.hpp>

//#define _DEBUG
#ifdef _DEBUG
#include <iostream>
#endif

/* For test purposes; if a dummies.dmap file is found in the folder from where the
 * program is being executed it would be used as dmap file. The default dmap file
 * would be DMAP_FILE_PATH.
 */
#define DMAP_FILE_PATH  "/usr/local/etc/mtca4u/devicefactory.dmap"
#define TEST_DMAP_FILE_PATH  "/dummies.dmap"
namespace mtca4u {

class DeviceFactoryException : public Exception {
public:
	enum {UNKNOWN_ALIAS,UNREGISTERED_DEVICE};
	DeviceFactoryException(const std::string &message, unsigned int exceptionID)
	: Exception( message, exceptionID ){}
};

/** DeviceFactory is a the factory class to create devices.
 * It is implemented as a Meyers' singleton.
 */
class DeviceFactory {
private:
	/** Add known device */
	DeviceFactory() {
		registerDeviceType("pci","",&PcieDevice::createInstance);
		registerDeviceType("pci","pcie",&PcieDevice::createInstance);
		registerDeviceType("fake","",&FakeDevice::createInstance);
		registerDeviceType("dummy","",&DummyDevice::createInstance);
	};

	boost::tuple<boost::shared_ptr<BaseDevice>, DMapFile::dmapElem> parseDMap(std::string devName);

	//DeviceFactory(DeviceFactory const&);     /** To avoid making copies */

	void operator=(DeviceFactory const&); /** To avoid making copies */

	/** Holds  device type and function pointer to the createInstance function of
	 * plugin*/

	std::map< std::pair<std::string, std::string>, boost::shared_ptr<mtca4u::BaseDevice> (*)(std::string host, std::string instance, std::list<std::string>parameters) > creatorMap;


public:
	/** This functions add new device using uri as a key. If a key already exist
	 * it replaces it*/
	void registerDeviceType(std::string interface, std::string protocol,
			boost::shared_ptr<BaseDevice> (*creatorFunction)(std::string host, std::string instance, std::list<std::string>parameters));

	/** Create a new device by calling the constructor and returning a pointer to
	 * the
	 * device */

	boost::shared_ptr<BaseDevice> createDevice(std::string devname);
	boost::shared_ptr < MappedDevice<BaseDevice> > createMappedDevice(std::string devname);

	/**Static function to get an instance of factory */
	static DeviceFactory& getInstance() {
#ifdef _DEBUG
		std::cout << "getInstance" << std::endl << std::flush;
#endif
		static DeviceFactory factoryInstance; /** Thread safe in C++11*/
		return factoryInstance;
	}
};

} // namespace mtca4u

#endif /* _MTCA4U_DEVICE_FACTORY_H__ */
