#ifndef MTCA4U_DUMMY_DEVICE_H
#define MTCA4U_DUMMY_DEVICE_H

#include <vector>
#include <map>
#include <list>
#include <set>

#include <boost/function.hpp>

#include "Exception.h"
#include "DeviceBackendImpl.h"
#include "RegisterInfoMap.h"

namespace mtca4u{

///fixme: there should only be one type of exception for all devices. Otherwise you will never be able to
/// interpret the enum in an exception from a pointer to BackendDevice.
class DummyDeviceException : public DeviceBackendException {
public:
	enum {WRONG_SIZE, ALREADY_OPEN, ALREADY_CLOSED, INVALID_ADDRESS};
	DummyDeviceException(const std::string &message, unsigned int exceptionID)
	: DeviceBackendException( message, exceptionID ){}
};


// foward declarations
template<typename T> class DummyRegisterAccessor;
template<typename T> class DummyMultiplexedRegisterAccessor;

/** The dummy device opens a mapping file instead of a device, and
 *  implements all registers defined in the mapping file im memory.
 *  Like this it mimiks the real PCIe device.
 *
 *  Deriving from this class, you can write dedicated implementations
 *  with special functionality.
 *  For this purpose one can register write callback function which are
 *  executed if a certain register (or range of registers) is written.
 *  For instance: Writing to a START_DAQ register
 *  can fill a data buffer with dummy values which can be read back.
 *  For each call of writeReg or writeArea the callback function is called once.
 *  If you require the callback function to be executed after each
 *  register change, use writeReg multiple times instead of writeArea.
 *
 *  Registers can be set to read-only mode. In this
 *  case a write operation will just be ignored and no callback
 *  function is executed.
 */
class DummyBackend : public DeviceBackendImpl
{
private:
	/** _mapFile is first and only item of arugment parameters  */
	std::string _mapFile;
protected:

	/** The file name has to be a mapping file, not a device file.
	 *  Permissons and config are ignored.
	 */
public:
	DummyBackend(std::string host, std::string instance, std::list<std::string> parameters);
	DummyBackend();
	virtual ~DummyBackend();

	virtual void open(const std::string &mappingFileName,
			int perm = O_RDWR, DeviceConfigBase* pConfig = NULL);
	virtual void open();

	/** This closes the device, clears all internal regsiters, read-only settings and
	 *  callback functions. As the device could be opened with another mapping file later,
	 *  these will most probably be invalid in this case. This is why the read-only  settings
	 *  and callback functions have to be set again when reopening the file.
	 */
	virtual void close();

	virtual void read(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes);
	virtual void write(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes);
	virtual void readDMA(uint8_t bar, uint32_t address, int32_t *data,  size_t sizeInBytes);
	virtual void writeDMA(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes);

	virtual std::string readDeviceInfo();

	/// A virtual address is an address is a virtual 64 bit address space
	/// which contains all bars.
	static uint64_t calculateVirtualAddress(
			uint32_t registerOffsetInBar,
			uint8_t bar);



	static boost::shared_ptr<DeviceBackend> createInstance(std::string host, std::string instance, std::list<std::string> parameters);

protected:
	struct AddressRange{
		const uint32_t offset;
		const uint32_t sizeInBytes;
		const uint8_t bar;
		AddressRange( uint8_t bar_, uint32_t address,  size_t sizeInBytes_ )
		: offset( address ), sizeInBytes( sizeInBytes_ ), bar( bar_ ){}
		bool operator<(AddressRange const & right) const {
			return ( bar == right.bar ? ( offset < right.offset ) : bar < right.bar );
		}
	};

	std::map< uint8_t, std::vector<int32_t> > _barContents;
	std::set< uint64_t > _readOnlyAddresses;
	std::multimap< AddressRange, boost::function<void(void)> > _writeCallbackFunctions;
	ptrmapFile _registerMapping;

	void resizeBarContents();
	std::map< uint8_t, size_t > getBarSizesInBytesFromRegisterMapping() const;
	void runWriteCallbackFunctionsForAddressRange( AddressRange addressRange );
	std::list< boost::function<void(void)> > findCallbackFunctionsForAddressRange(AddressRange addressRange);
	void setReadOnly( uint8_t bar, uint32_t address, size_t sizeInWords);
	void setReadOnly( AddressRange addressRange);
	bool isReadOnly( uint8_t bar, uint32_t address ) const;
	void setWriteCallbackFunction( AddressRange addressRange,
			boost::function<void(void)>  const & writeCallbackFunction );
	/// returns true if the ranges overlap and at least one of the overlapping registers can be written
	bool isWriteRangeOverlap( AddressRange firstRange, AddressRange secondRange);
	static void checkSizeIsMultipleOfWordSize(size_t sizeInBytes);

	/// Not write-protected function for internal use only. It does not trigger
	/// the callback function so it can be used inside a callback function for
	/// resynchronisation.
	void writeRegisterWithoutCallback(uint8_t bar, uint32_t address, int32_t data);

        /// register accessors must be friends to access the map and the registers
        template<typename T>
        friend class DummyRegisterAccessor;

        template<typename T>
        friend class DummyMultiplexedRegisterAccessor;

};

}//namespace mtca4u

#endif // MTCA4U_DUMMY_DEVICE_H
