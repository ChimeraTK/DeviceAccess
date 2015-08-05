#ifndef _MTCA4U_DEVICE_FACTORY_H__
#define _MTCA4U_DEVICE_FACTORY_H__

#include "FakeDevice.h"
#include "BaseDevice.h"
#include "DummyDevice.h"
#include <fstream>
#include <string>
#include <map>
#include <list>
#include <boost/tuple/tuple.hpp>

#include "MappedDevice.h"
#include "PcieDevice.h"

//#define _DEBUG
#ifdef _DEBUG
#include <iostream>
#endif

#define DMAP_FILE_PATH "../tests/dummies.dmap" /*path has to be defined*/
//#define DMAP_FILE_PATH // "/space/nshehzad/work/MtcaMappedDevice/build_ni/tests/dummies.dmap"
namespace mtca4u {

class DeviceFactoryException : public ExcBase {
public:
	enum {UNKNOWN_ALIAS,UNREGISTERED_DEVICE};
	DeviceFactoryException(const std::string &message, unsigned int exceptionID)
	: ExcBase( message, exceptionID ){}
};

/** devFactory is a the factory class to create devices.
 * It is implemented as a Meyers' singleton.
 */
class DeviceFactory {
private:
	/** Add known device */
	DeviceFactory() {
		registerDevice("pci","",&PcieDevice::createInstance);
		registerDevice("pci","pcie",&PcieDevice::createInstance);
		registerDevice("fake","",&FakeDevice::createInstance);
		registerDevice("dummy","",&DummyDevice::createInstance);
	};

	boost::tuple<BaseDevice*, DMapFile::dmapElem> parseDMap(std::string devName);

	//DeviceFactory(DeviceFactory const&);     /** To avoid making copies */

	void operator=(DeviceFactory const&); /** To avoid making copies */

	/** Holds  device type and function pointer to the createInstance function of
	 * plugin*/

	std::map< std::pair<std::string, std::string>, BaseDevice* (*)(std::string host, std::string instance, std::list<std::string>parameters) > creatorMap;


public:
	/** This functions add new device using uri as a key. If a key already exist
	 * it replaces it*/
	void registerDevice(std::string interface, std::string protocol,
			BaseDevice* (*creatorFunction)(std::string host, std::string instance, std::list<std::string>parameters));

	/** Create a new device by calling the constructor and returning a pointer to
	 * the
	 * device */

	MappedDevice<BaseDevice>* createMappedDevice(std::string devname);
	BaseDevice* createDevice(std::string devname);

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
