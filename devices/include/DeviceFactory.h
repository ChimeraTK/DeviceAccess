#ifndef DeviceFactory_H
#define DeviceFactory_H

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

#define DMAP_FILE_PATH "../tests/dummies.dmap"
/*#define DMAP_FILE_PATH                                                         \
  "/space/nshehzad/work/MtcaMappedDevice/build/tests/dummies.dmap"*/
namespace mtca4u {

/** devFactory is a the factory class to create devices.
 * It is implemented as a Meyers' singleton.
 */
class DeviceFactory {
private:
	/** Add known device */
	DeviceFactory() {
		std::list<std::string> parameters;
		parameters.push_back("ip=128.101.9.10");
		parameters.push_back("netmask=255.255.240.0");
		addDevice(".", "Asequences2", "0", "s0", parameters,
				&DummyDevice::createInstance);
		parameters.clear();
		addDevice(".", "devPcie", "0", "S1", parameters,
				&PcieDevice::createInstance);
		addDevice(".", "dev/pcieunidummy", "0", "s6", parameters,
				&PcieDevice::createInstance);
		addDevice(".", "dev/llrfdummy", "0", "s4", parameters,
				&PcieDevice::createInstance);
		addDevice(".", "dev/mtcadummy", "0", "s0", parameters,
				&PcieDevice::createInstance);
		addDevice(".", "Reference_Device", "0", "", parameters,
				&FakeDevice::createInstance); // fake
		addDevice(".", "fakeDevice", "0", "", parameters,
				&FakeDevice::createInstance); // fake
		addDevice(".", "DummyDevice", "0", "", parameters,
				&FakeDevice::createInstance); // fake
		addDevice(".", "sequences", "0", "", parameters,
				&DummyDevice::createInstance);
		addDevice(".", "../tests/mtcadummy_withoutModules.map", "0", "", parameters,
				&DummyDevice::createInstance);
	};

	boost::tuple<BaseDevice*, DMapFile::dmapElem> parseDMap(std::string devname);

	//DeviceFactory(DeviceFactory const&);     /** To avoid making copies */

	void operator=(DeviceFactory const&); /** To avoid making copies */

	/** Holds  device type and function pointer to the createInstance function of
	 * plugin*/
	std::map<std::string, BaseDevice* (*)(std::string devName)> creatorMap;

	std::string uriToNode(std::string uri); /** Converts uri to device name or file */

public:
	/** This functions add new device using uri as a key. If a key already exist
	 * it replaces it*/
	void addDevice(std::string HostId, std::string interface,
			std::string instance, std::string protocol,
			std::list<std::string> parameters,
			BaseDevice* (*creatorFunction)(std::string devName)) {
#ifdef _DEBUG
		std::cout << "adding:" << interface << std::endl << std::flush;
#endif
		std::string sdm = "sdm";
		std::string slash = "/";
		std::string colon = ":";
		std::string comma = ",";
		std::string parameterString = "";
		if (parameters.size() > 0)
			parameterString = ";";
		for (std::list<std::string>::iterator iter = parameters.begin();
				iter != parameters.end(); ++iter) {
			parameterString +=
					/*std::string(" ") + parser does not support space
					 */ *iter;
		}
		creatorMap[sdm + colon + slash + slash + HostId + slash + interface +
							 comma + instance + colon + protocol + parameterString] =
									 creatorFunction;
	}

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

#endif /* DeviceFactory_H */
