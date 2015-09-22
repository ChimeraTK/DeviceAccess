#ifndef MTCA4U_MAPPEDDEVICE_H
#define MTCA4U_MAPPEDDEVICE_H

/**
 *      @file           Device.h
 *      @author         Adam Piotrowski <adam.piotrowski@desy.de>
 *      @brief          Template that connect functionality of libdev and libmap
 *libraries.
 *                      This file support only map file parsing.
 *
 */

#include "DeviceException.h"
#include "FixedPointConverter.h"
#include "MultiplexedDataAccessor.h"
#include "BackendFactory.h"
#include <boost/algorithm/string.hpp>
#include <boost/shared_ptr.hpp>
namespace mtca4u {

/**
 *      @class  Device
 *      @brief  Class allows to read/write registers from device
 *
 *      Allows to read/write registers from device by passing the name of
 *      the register instead of offset from the beginning of address space.
 *      Type of the object used to control access to device must be passed
 *      as a template parameter and must be an type defined in libdev class.
 *
 *      The device can open and close a device for you. If you let the Device
 *open
 *      the device you will not be able to get a handle to this device
 *directly,
 *you
 *      can only close it with the Device. Should you create RegisterAccessor
 *objects, which contain
 *      shared pointers to this device, the device will stay opened and
 *functional even
 *      if the Device object which created the RegisterAccessor goes out of
 *scope. In this case
 *      you cannot close the device. It will finally be closed when the the
 *last
 *      RegisterAccessor pointing to it goes out if scope.
 *      The same holds if you open another device with the same Device: You
 *lose
 *direct access
 *      to the previous device, which stays open as long as there are
 *RegisterAccessors pointing to it.
 *
 */
class Device {
public:
	typedef boost::shared_ptr<DeviceBackend> _ptrDeviceBackend;

private:
	_ptrDeviceBackend _pDeviceBackend;
	std::string _mapFileName;
	ptrmapFile _registerMap;

public:
	class RegisterAccessor {
		RegisterInfoMap::RegisterInfo _registerInfo;
		typename Device::_ptrDeviceBackend _pDeviceBackend;
		FixedPointConverter _fixedPointConverter;

	private:
		static void checkRegister(const RegisterInfoMap::RegisterInfo &me, size_t dataSize,
				uint32_t addRegOffset, uint32_t &retDataSize,
				uint32_t &retRegOff);

	public:
		RegisterAccessor(const RegisterInfoMap::RegisterInfo &_me,
				typename Device::_ptrDeviceBackend pDeviceBackend);

		/** Read one ore more words from the device. It calls DeviceBackend::readArea,
		 * not
		 * DeviceBackend::readReg.
		 *  @attention In case you leave data size at 0, the full size of the
		 * register is read, not just one
		 *  word as in DeviceBackend::readArea! Make sure your buffer is large enough!
		 */
		void readRaw(int32_t *data, size_t dataSize = 0,
				uint32_t addRegOffset = 0) const;

		/** Write one ore more words to the device. It calls DeviceBackend::readArea,
		 * not
		 * DeviceBackend::writeReg.
		 *  @attention In case you leave data size at 0, the full size of the
		 * register is read, not just one
		 *  word as in DeviceBackend::readArea! Make sure your buffer is large enough!
		 */
		void writeRaw(int32_t const *data, size_t dataSize = 0,
				uint32_t addRegOffset = 0);

		void readDMA(int32_t *data, size_t dataSize = 0,
				uint32_t addRegOffset = 0) const;

		void writeDMA(int32_t const *data, size_t dataSize = 0,
				uint32_t addRegOffset = 0);

		/** Read (a block of) values with automatic data conversion. The first
		 *parameter is a pointer to
		 *  to the output buffer. It is templated to work with basic data types.
		 *Implementations exist for
		 *  \li int32_t
		 *  \li uint32_t
		 *  \li int16_t
		 *  \li uint16_t
		 *  \li int8_t
		 *  \li uint8_t
		 *  \li float
		 *  \li double
		 *
		 *  Note that the input is always a 32 bit word, which is being
		 *interpreted
		 *to be one
		 *  output word. It is not possible to do conversion e.g. from one 32 bit
		 *word to two 16 bit values.
		 *
		 *  @attention Be aware of rounding errors and range overflows, depending
		 *on
		 *the data type.
		 *  \li Rounding to integers is done correctly, so a fixed point value of
		 *3.75 would be converted to 4.
		 *  \li Coversion to double is guaranteed to be exact (32 bit fixed point
		 *with fractional bits
		 *  -1024 to 1023 is guaranteed by the FixedPointConverter).
		 *  \li Conversion to float is exact for fixed point values up to 24 bits
		 *and fractional bits from
		 *  -128 to 127.
		 */
		template <typename ConvertedDataType>
		void read(ConvertedDataType *convertedData, size_t nWords = 1,
				uint32_t wordOffsetInRegister = 0) const;

		/** Convenience function to read a single word. It allows shorter syntax
		 *  as the read value is the return value and one does not have to pass a
		 *pointer.
		 *
		 *  Example: You can use
		 *  \code
		 *  uint16_t i = registerAccessor.read<uint16_t>();
		 *  \endcode
		 *  instead of
		 *  \code
		 *  uint16_t i;
		 *  registerAccessor.read(&i);
		 *  \endcode
		 *  Note that you have to specify the data type as template parameter
		 *because return type
		 *  overloading is not supported in C++.
		 */
		template <typename ConvertedDataType> ConvertedDataType read() const;

		/** Write (a block of) words with automatic data conversion. It works for
		 *every data
		 *  type which has an implicit conversion to double (tested with all data
		 *types which are
		 *  implemented for read()).
		 *  Each input word will be converted to a fixed point integer and written
		 *to
		 *  a 32 bit register.
		 *
		 *  @attention Be aware that the conversion to fixed point might come with
		 *a
		 *loss of
		 *  precision or range overflows!
		 *
		 *  The nWords option does not have a default value to keep the template
		 *signature different from
		 *  the single word convenience write function.
		 */
		template <typename ConvertedDataType>
		void write(ConvertedDataType const *convertedData, size_t nWords,
				uint32_t wordOffsetInRegister = 0);

		/** Convenience function for single words. The value can be given
		 * directly,
		 * no need to
		 *  have a an instance and a pointer for it. This allows code like
		 *  \code
		 *  registerAccessor.write(0x3F);
		 *  \endcode
		 *  instead of
		 *  \code
		 *  static const uint32_t tempValue = 0x3F;
		 *  registerAccessor.write(&tempValue); // defaulting nWords to 1 would be
		 * possible if this function did not exist
		 *  \endcode
		 */
		template <typename ConvertedDataType>
		void write(ConvertedDataType const &convertedData);

		/** Returns the register information aka RegisterInfo.
		 *  This function was named getRegisterInfo because RegisterInfo will be
		 * renamed.
		 */
		RegisterInfoMap::RegisterInfo const &getRegisterInfo() const;

		/** Return's a reference to the correctly configured internal fixed point
		 *  converter for the register
		 */
		FixedPointConverter const &getFixedPointConverter() const;
	};

	/** A typedef for backward compatibility.
	 *  @deprecated Don't use this in new code. It will be removed in a future
	 * release.
	 *  Use RegisterAccessor instead.
	 */
	typedef RegisterAccessor regObject;

	Device(){};

	virtual void open(std::string const & aliasName);

	virtual void open(boost::shared_ptr<DeviceBackend> deviceBackend, boost::shared_ptr<RegisterInfoMap> registerInfoMap);

	virtual void close();
	virtual void readReg(uint32_t regOffset, int32_t *data, uint8_t bar) const;
	virtual void writeReg(uint32_t regOffset, int32_t data, uint8_t bar);
	virtual void readArea(uint32_t regOffset, int32_t *data, size_t size,
			uint8_t bar) const;
	virtual void writeArea(uint32_t regOffset, int32_t const *data, size_t size,
			uint8_t bar);
	virtual void readDMA(uint32_t regOffset, int32_t *data, size_t size,
			uint8_t bar) const;
	virtual void writeDMA(uint32_t regOffset, int32_t const *data, size_t size,
			uint8_t bar);
	virtual std::string readDeviceInfo() const;

	/** Read one or more words from the device. It calls DeviceBackend::readArea, not
	 * DeviceBackend::readRaw.
	 *  @attention In case you leave data size at 0, the full size of the
	 * register
	 * is read, not just one
	 *  word as in DeviceBackend::readArea! Make sure your buffer is large enough!
	 *  The original readRaw function without module name, kept for backward
	 * compatibility.
	 *  The signature was changed and not extendet to keep the functionality of
	 * the default parameters for
	 *  dataSize and addRegOffset, as the module name will always be needed in
	 * the
	 * future.
	 */
	virtual void readReg(const std::string &regName, int32_t *data,
			size_t dataSize = 0, uint32_t addRegOffset = 0) const;
	/** Read one or more words from the device. It calls DeviceBackend::readArea, not
	 * DeviceBackend::readRaw.
	 *  @attention In case you leave data size at 0, the full size of the
	 * register
	 * is read, not just one
	 *  word as in DeviceBackend::readArea! Make sure your buffer is large enough!
	 */
	virtual void readReg(const std::string &regName, const std::string &regModule,
			int32_t *data, size_t dataSize = 0,
			uint32_t addRegOffset = 0) const;

	/** Write one or more words from the device. It calls DeviceBackend::writeArea,
	 * not
	 * DeviceBackend::writeRaw.
	 *  @attention In case you leave data size at 0, the full size of the
	 * register
	 * is written, not just one
	 *  word as in DeviceBackend::readArea! Make sure your buffer is large enough!
	 *  The original writeRaw function without module name, kept for backward
	 * compatibility.
	 *  The signature was changed and not extendet to keep the functionality of
	 * the default parameters for
	 *  dataSize and addRegOffset, as the module name will always be needed in
	 * the
	 * future.
	 */
	virtual void writeReg(const std::string &regName, int32_t const *data,
			size_t dataSize = 0, uint32_t addRegOffset = 0);
	/** Write one or more words from the device. It calls DeviceBackend::writeArea,
	 * not
	 * DeviceBackend::writeRaw.
	 *  @attention In case you leave data size at 0, the full size of the
	 * register
	 * is written, not just one
	 *  word as in DeviceBackend::readArea! Make sure your buffer is large enough!
	 */
	virtual void writeReg(const std::string &regName,
			const std::string &regModule, int32_t const *data,
			size_t dataSize = 0, uint32_t addRegOffset = 0);

	/** Read a block of data via DMA.
	 *  @attention In case you leave data size at 0, the full size of the
	 * register
	 * is read.
	 *  Make sure your buffer is large enough!
	 *  The original readDMA function without module name, kept for backward
	 * compatibility.
	 *  The signature was changed and not extendet to keep the functionality of
	 * the default parameters for
	 *  dataSize and addRegOffset, as the module name will always be needed in
	 * the
	 * future.
	 */
	virtual void readDMA(const std::string &regName, int32_t *data,
			size_t dataSize = 0, uint32_t addRegOffset = 0) const;

	/** Read a block of data via DMA.
	 *  @attention In case you leave data size at 0, the full size of the
	 * register
	 * is read.
	 *  Make sure your buffer is large enough!
	 */
	virtual void readDMA(const std::string &regName, const std::string &regModule,
			int32_t *data, size_t dataSize = 0,
			uint32_t addRegOffset = 0) const;

	/** Write a block of data via DMA.
	 *  @attention In case you leave data size at 0, the full size of the
	 * register
	 * is written.
	 *  Make sure your buffer is large enough!
	 *  The original writeDMA function without module name, kept for backward
	 * compatibility.
	 *  The signature was changed and not extendet to keep the functionality of
	 * the default parameters for
	 *  dataSize and addRegOffset, as the module name will always be needed in
	 * the
	 * future.
	 */
	virtual void writeDMA(const std::string &regName, int32_t const *data,
			size_t dataSize = 0, uint32_t addRegOffset = 0);
	/** Write a block of data via DMA.
	 *  @attention In case you leave data size at 0, the full size of the
	 * register
	 * is written.
	 *  Make sure your buffer is large enough!
	 */
	virtual void writeDMA(const std::string &regName,
			const std::string &regModule, int32_t const *data,
			size_t dataSize = 0, uint32_t addRegOffset = 0);

	/** Get a regObject from the register name.
	 *  @deprecated Use getRegisterAccessor instead.
	 */
	regObject getRegObject(const std::string &regName) const;

	/** Get a RegisterAccessor object from the register name.
	 */
	boost::shared_ptr<RegisterAccessor> getRegisterAccessor(
			const std::string &registerName,
			const std::string &module = std::string()) const;

	/**
	 * returns an accssesor which is used for interpreting  data contained in a
	 * memory region. Memory regions that require a custom interpretation would
	 * be indicated by specific keywords in the mapfile. For example, a memory
	 * region indicated by the keyword
	 * AREA_MULTIPLEXED_SEQUENCE_<dataRegionName> indicates that this memory
	 * region contains multiplexed data sequences. The intelligence for
	 * interpreting this multiplexed data is provided through the custom class -
	 * MultiplexedDataAccessor<userType>
	 */
	template <typename customClass>
	boost::shared_ptr<customClass> getCustomAccessor(
			const std::string &dataRegionName,
			const std::string &module = std::string()) const;

	/** Get a complete list of RegisterInfo objects (mapfile::RegisterInfo) for one
	 * module.
	 *  The registers are in alphabetical order.
	 */
	std::list<RegisterInfoMap::RegisterInfo> getRegistersInModule(
			const std::string &moduleName) const;

	/** Get a complete list of RegisterAccessors for one module.
	 *  The registers are in alphabetical order.
	 */
	std::list<RegisterAccessor> getRegisterAccessorsInModule(
			const std::string &moduleName) const;

	/** Returns the register information aka RegisterInfo.
	 *  This function was named getRegisterMap because RegisterInfoMap will be renamed.
	 */
	boost::shared_ptr<const RegisterInfoMap> getRegisterMap() const;

	virtual ~Device();

private:
	void checkRegister(const std::string &regName,
			const std::string &registerModule, size_t dataSize,
			uint32_t addRegOffset, uint32_t &retDataSize,
			uint32_t &retRegOff, uint8_t &retRegBar) const;

	void checkPointersAreNotNull() const;
};


template <typename ConvertedDataType>
ConvertedDataType Device::RegisterAccessor::read() const {
	ConvertedDataType t;
	read(&t);
	return t;
}

template <typename ConvertedDataType>
void Device::RegisterAccessor::read(ConvertedDataType * convertedData, size_t nWords,
		uint32_t wordOffsetInRegister) const {
	if (nWords==0){
		return;
	}

	std::vector<int32_t> rawDataBuffer(nWords);
	readRaw(&(rawDataBuffer[0]), nWords*sizeof(int32_t), wordOffsetInRegister*sizeof(int32_t));

	for(size_t i=0; i < nWords; ++i){
		convertedData[i] = _fixedPointConverter.template toCooked<ConvertedDataType>(rawDataBuffer[i]);
	}
}


template <typename ConvertedDataType>
void Device::RegisterAccessor::write(ConvertedDataType const *convertedData,
		size_t nWords,
		uint32_t wordOffsetInRegister) {
	// Check that nWords is not 0. The readRaw command would read the whole
	// register, which
	// will the raw buffer size of 0.
	if (nWords == 0) {
		return;
	}

	std::vector<int32_t> rawDataBuffer(nWords);
	for (size_t i = 0; i < nWords; ++i) {
		rawDataBuffer[i] = _fixedPointConverter.toRaw(convertedData[i]);
	}
	writeRaw(&(rawDataBuffer[0]), nWords * sizeof(int32_t),
			wordOffsetInRegister * sizeof(int32_t));
}


template <typename ConvertedDataType>
void Device::RegisterAccessor::write(
		ConvertedDataType const &convertedData) {
	write(&convertedData, 1);
}


template <typename customClass>
boost::shared_ptr<customClass> Device::getCustomAccessor(
		const std::string &dataRegionName, const std::string &module) const {
	return (
			customClass::createInstance(dataRegionName, module, _pDeviceBackend, _registerMap));
}
} // namespace mtca4u

#endif /* MTCA4U_MAPPEDDEVICE_H */
