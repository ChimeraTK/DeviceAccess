#ifndef MTCA4U_MAPPEDDEVICE_H
#define MTCA4U_MAPPEDDEVICE_H

/**
 *      @file           MappedDevice.h
 *      @author         Adam Piotrowski <adam.piotrowski@desy.de>
 *      @brief          Template that connect functionality of libdev and libmap
 *libraries.
 *                      This file support only map file parsing.
 *
 */

#include <boost/shared_ptr.hpp>

#include "libmap.h"
// #include "libdev.h"
#include "MappedDeviceException.h"
#include "FixedPointConverter.h"
#include "MultiplexedDataAccessor.h"
namespace mtca4u {

/**
 *      @class  MappedDevice
 *      @brief  Class allows to read/write registers from device
 *
 *      Allows to read/write registers from device by passing the name of
 *      the register instead of offset from the beginning of address space.
 *      Type of the object used to control access to device must be passed
 *      as a template parameter and must be an type defined in libdev class.
 *
 *      The device can open and close a device for you. If you let the MappedDevice
 *open
 *      the device you will not be able to get a handle to this device
 *directly,
 *you
 *      can only close it with the MappedDevice. Should you create RegisterAccessor
 *objects, which contain
 *      shared pointers to this device, the device will stay opened and
 *functional even
 *      if the MappedDevice object which created the RegisterAccessor goes out of
 *scope. In this case
 *      you cannot close the device. It will finally be closed when the the
 *last
 *      RegisterAccessor pointing to it goes out if scope.
 *      The same holds if you open another device with the same MappedDevice: You
 *lose
 *direct access
 *      to the previous device, which stays open as long as there are
 *RegisterAccessors pointing to it.
 *
 */
template <typename T> class MappedDevice {

public:
  typedef boost::shared_ptr<T> ptrdev;

private:
  ptrdev pdev;
  std::string mapFileName;
  ptrmapFile registerMap;

public:
  class RegisterAccessor {
    mapFile::mapElem me;
    typename MappedDevice::ptrdev pdev;
    FixedPointConverter _fixedPointConverter;

  private:
    static void checkRegister(const mapFile::mapElem &me, size_t dataSize,
                              uint32_t addRegOffset, uint32_t &retDataSize,
                              uint32_t &retRegOff);

  public:
    RegisterAccessor(const std::string & /*_regName*/, // not needed, info is
                     // already in the mapElem
                     const mapFile::mapElem &_me,
                     typename MappedDevice::ptrdev _pdev);

    /** Read one ore more words from the device. It calls BaseDevice::readArea,
     * not
     * BaseDevice::readReg.
     *  @attention In case you leave data size at 0, the full size of the
     * register is read, not just one
     *  word as in BaseDevice::readArea! Make sure your buffer is large enough!
     */
    void readReg(int32_t *data, size_t dataSize = 0,
                 uint32_t addRegOffset = 0) const;

    /** Write one ore more words to the device. It calls BaseDevice::readArea,
     * not
     * BaseDevice::readReg.
     *  @attention In case you leave data size at 0, the full size of the
     * register is read, not just one
     *  word as in BaseDevice::readArea! Make sure your buffer is large enough!
     */
    void writeReg(int32_t const *data, size_t dataSize = 0,
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

    /** Returns the register information aka mapElem.
     *  This function was named getRegisterInfo because mapElem will be
     * renamed.
     */
    mapFile::mapElem const &getRegisterInfo() const;

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

  MappedDevice(){};
  MappedDevice(boost::shared_ptr<mtca4u::BaseDevice> baseDevice, const std::string & mapFileName);

  virtual void openDev(const std::string &_devFileName,
                       const std::string &_mapFileName, int _perm = O_RDWR,
                       DeviceConfigBase *_pConfig = NULL);
  virtual void openDev(
      std::pair<std::string, std::string> const &_deviceFileAndMapFileName,
      int _perm = O_RDWR, DeviceConfigBase *_pConfig = NULL);
  virtual void openDev(ptrdev ioDevice, ptrmapFile registerMapping);
  virtual void closeDev();
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
  virtual void readDeviceInfo(std::string *devInfo) const;

  /** Read one or more words from the device. It calls BaseDevice::readArea, not
   * BaseDevice::readReg.
   *  @attention In case you leave data size at 0, the full size of the
   * register
   * is read, not just one
   *  word as in BaseDevice::readArea! Make sure your buffer is large enough!
   *  The original readReg function without module name, kept for backward
   * compatibility.
   *  The signature was changed and not extendet to keep the functionality of
   * the default parameters for
   *  dataSize and addRegOffset, as the module name will always be needed in
   * the
   * future.
   */
  virtual void readReg(const std::string &regName, int32_t *data,
                       size_t dataSize = 0, uint32_t addRegOffset = 0) const;
  /** Read one or more words from the device. It calls BaseDevice::readArea, not
   * BaseDevice::readReg.
   *  @attention In case you leave data size at 0, the full size of the
   * register
   * is read, not just one
   *  word as in BaseDevice::readArea! Make sure your buffer is large enough!
   */
  virtual void readReg(const std::string &regName, const std::string &regModule,
                       int32_t *data, size_t dataSize = 0,
                       uint32_t addRegOffset = 0) const;

  /** Write one or more words from the device. It calls BaseDevice::writeArea,
   * not
   * BaseDevice::writeReg.
   *  @attention In case you leave data size at 0, the full size of the
   * register
   * is written, not just one
   *  word as in BaseDevice::readArea! Make sure your buffer is large enough!
   *  The original writeReg function without module name, kept for backward
   * compatibility.
   *  The signature was changed and not extendet to keep the functionality of
   * the default parameters for
   *  dataSize and addRegOffset, as the module name will always be needed in
   * the
   * future.
   */
  virtual void writeReg(const std::string &regName, int32_t const *data,
                        size_t dataSize = 0, uint32_t addRegOffset = 0);
  /** Write one or more words from the device. It calls BaseDevice::writeArea,
   * not
   * BaseDevice::writeReg.
   *  @attention In case you leave data size at 0, the full size of the
   * register
   * is written, not just one
   *  word as in BaseDevice::readArea! Make sure your buffer is large enough!
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

  /** Get a complete list of RegisterInfo objects (mapfile::mapElem) for one
   * module.
   *  The registers are in alphabetical order.
   */
  std::list<mapFile::mapElem> getRegistersInModule(
      const std::string &moduleName) const;

  /** Get a complete list of RegisterAccessors for one module.
   *  The registers are in alphabetical order.
   */
  std::list<RegisterAccessor> getRegisterAccessorsInModule(
      const std::string &moduleName) const;

  /** Returns the register information aka mapElem.
   *  This function was named getRegisterMap because mapFile will be renamed.
   */
  boost::shared_ptr<const mapFile> getRegisterMap() const;

  virtual ~MappedDevice();

private:
  void checkRegister(const std::string &regName,
                     const std::string &registerModule, size_t dataSize,
                     uint32_t addRegOffset, uint32_t &retDataSize,
                     uint32_t &retRegOff, uint8_t &retRegBar) const;

  void checkPointersAreNotNull() const;
};



template <typename T> MappedDevice<T>::~MappedDevice() {
  // FIXME: do we want to close here? It will probably leave not working
  // RegisterAccessors
  // if(pdev) pdev->closeDev();
}


template <typename T>
boost::shared_ptr<const mapFile> MappedDevice<T>::getRegisterMap() const {
  return registerMap;
}

template <typename T>
typename MappedDevice<T>::RegisterAccessor MappedDevice<T>::getRegObject(
    const std::string &regName) const {
  checkPointersAreNotNull();

  mapFile::mapElem me;
  registerMap->getRegisterInfo(regName, me);
  return MappedDevice::RegisterAccessor(regName, me, pdev);
}

template <typename T>
boost::shared_ptr<typename MappedDevice<T>::RegisterAccessor>
MappedDevice<T>::getRegisterAccessor(const std::string &regName,
                               const std::string &module) const {
  checkPointersAreNotNull();

  mapFile::mapElem me;
  registerMap->getRegisterInfo(regName, me, module);
  return boost::shared_ptr<typename MappedDevice<T>::RegisterAccessor>(
      new MappedDevice::RegisterAccessor(regName, me, pdev));
}

template <typename T>
typename std::list<mapFile::mapElem> MappedDevice<T>::getRegistersInModule(
    const std::string &moduleName) const {
  checkPointersAreNotNull();

  return registerMap->getRegistersInModule(moduleName);
}

template <typename T>
std::list<typename MappedDevice<T>::RegisterAccessor>
MappedDevice<T>::getRegisterAccessorsInModule(const std::string &moduleName) const {
  checkPointersAreNotNull();

  std::list<mapFile::mapElem> registerInfoList =
      registerMap->getRegistersInModule(moduleName);

  std::list<RegisterAccessor> accessorList;
  for (std::list<mapFile::mapElem>::const_iterator regInfo =
           registerInfoList.begin();
       regInfo != registerInfoList.end(); ++regInfo) {
    accessorList.push_back(
        MappedDevice::RegisterAccessor(regInfo->reg_name, *regInfo, pdev));
  }

  return accessorList;
}

template <typename T>
void MappedDevice<T>::checkRegister(const std::string &regName,
                              const std::string &regModule, size_t dataSize,
                              uint32_t addRegOffset, uint32_t &retDataSize,
                              uint32_t &retRegOff, uint8_t &retRegBar) const {
  checkPointersAreNotNull();

  mapFile::mapElem me;
  registerMap->getRegisterInfo(regName, me, regModule);
  if (addRegOffset % 4) {
    throw MappedDeviceException("Register offset must be divisible by 4",
                   MappedDeviceException::EX_WRONG_PARAMETER);
  }
  if (dataSize) {
    if (dataSize % 4) {
      throw MappedDeviceException("Data size must be divisible by 4",
                     MappedDeviceException::EX_WRONG_PARAMETER);
    }
    if (dataSize > me.reg_size - addRegOffset) {
      throw MappedDeviceException("Data size exceed register size",
                     MappedDeviceException::EX_WRONG_PARAMETER);
    }
    retDataSize = dataSize;
  } else {
    retDataSize = me.reg_size;
  }
  retRegBar = me.reg_bar;
  retRegOff = me.reg_address + addRegOffset;
}

template <typename T>
void MappedDevice<T>::readReg(const std::string &regName, int32_t *data,
                        size_t dataSize, uint32_t addRegOffset) const {
  readReg(regName, std::string(), data, dataSize, addRegOffset);
}

template <typename T>
void MappedDevice<T>::readReg(const std::string &regName,
                        const std::string &regModule, int32_t *data,
                        size_t dataSize, uint32_t addRegOffset) const {
  uint32_t retDataSize;
  uint32_t retRegOff;
  uint8_t retRegBar;

  checkRegister(regName, regModule, dataSize, addRegOffset, retDataSize,
                retRegOff, retRegBar);
  readArea(retRegOff, data, retDataSize, retRegBar);
}

template <typename T>
void MappedDevice<T>::writeReg(const std::string &regName, int32_t const *data,
                         size_t dataSize, uint32_t addRegOffset) {
  writeReg(regName, std::string(), data, dataSize, addRegOffset);
}

template <typename T>
void MappedDevice<T>::writeReg(const std::string &regName,
                         const std::string &regModule, int32_t const *data,
                         size_t dataSize, uint32_t addRegOffset) {
  uint32_t retDataSize;
  uint32_t retRegOff;
  uint8_t retRegBar;

  checkRegister(regName, regModule, dataSize, addRegOffset, retDataSize,
                retRegOff, retRegBar);
  writeArea(retRegOff, data, retDataSize, retRegBar);
}

template <typename T>
void MappedDevice<T>::readDMA(const std::string &regName, int32_t *data,
                        size_t dataSize, uint32_t addRegOffset) const {
  readDMA(regName, std::string(), data, dataSize, addRegOffset);
}

template <typename T>
void MappedDevice<T>::readDMA(const std::string &regName,
                        const std::string &regModule, int32_t *data,
                        size_t dataSize, uint32_t addRegOffset) const {
  uint32_t retDataSize;
  uint32_t retRegOff;
  uint8_t retRegBar;

  checkRegister(regName, regModule, dataSize, addRegOffset, retDataSize,
                retRegOff, retRegBar);
  if (retRegBar != 0xD) {
    throw MappedDeviceException("Cannot read data from register \"" + regName +
                       "\" through DMA",
                   MappedDeviceException::EX_WRONG_PARAMETER);
  }
  readDMA(retRegOff, data, retDataSize, retRegBar);
}

template <typename T>
void MappedDevice<T>::writeDMA(const std::string &regName, int32_t const *data,
                         size_t dataSize, uint32_t addRegOffset) {
  writeDMA(regName, std::string(), data, dataSize, addRegOffset);
}

template <typename T>
void MappedDevice<T>::writeDMA(const std::string &regName,
                         const std::string &regModule, int32_t const *data,
                         size_t dataSize, uint32_t addRegOffset) {
  uint32_t retDataSize;
  uint32_t retRegOff;
  uint8_t retRegBar;
  checkRegister(regName, regModule, dataSize, addRegOffset, retDataSize,
                retRegOff, retRegBar);
  if (retRegBar != 0xD) {
    throw MappedDeviceException("Cannot read data from register \"" + regName +
                       "\" through DMA",
                   MappedDeviceException::EX_WRONG_PARAMETER);
  }
  writeDMA(retRegOff, data, retDataSize, retRegBar);
}

/**
 *      @brief  Function allows to open device specified by the name
 *
 *      Function throws the same exceptions like openDev from class type
 *      passed as a template parameter.
 *
 *      @param  _devFileName - name of the device
 *      @param  _mapFileName -  name of the map file string information about
 *                              registers available in device memory space.
 *      @param  _perm        -  permitions for the device file in form
 *accepted
 *                              by standard open function [default: O_RDWR]
 *      @param  _pConfig     -  additional configuration used to prepare
 *device.
 *                              Structure of this parameter depends on type of
 *                              the device [default: NULL]
 */
template <typename T>
void MappedDevice<T>::openDev(const std::string &_devFileName,
                        const std::string &_mapFileName, int _perm,
                        DeviceConfigBase *_pConfig) {
  mapFileParser fileParser;
  mapFileName = _mapFileName;
  registerMap = fileParser.parse(mapFileName);
  pdev.reset(new T);
  pdev->openDev(_devFileName, _perm, _pConfig);
}

/** Specialisation of openDev to be able to instantiate MappedDevice<BaseDevice>.
 *  To be removed when the templatisation of MappedDevice is removed.
 */
template <>
void MappedDevice<BaseDevice>::openDev(const std::string & /*_devFileName*/,
                                 const std::string & /*_mapFileName*/,
                                 int /*_perm*/,
                                 DeviceConfigBase * /*_pConfig*/);

/** Alternative open function where the two reqired file names are packed in
 * one
 * object (a pair), so it can be the return value of a single function call.
 * For parameters see openDev(const std::string &_devFileName, const
 * std::string& _mapFileName, int _perm, DeviceConfigBase* _pConfig);
 */
template <typename T>
void MappedDevice<T>::openDev(
    std::pair<std::string, std::string> const &_deviceFileAndMapFileName,
    int _perm, DeviceConfigBase *_pConfig) {
  openDev(_deviceFileAndMapFileName.first,  // the device file name
          _deviceFileAndMapFileName.second, // the map file name
          _perm, _pConfig);
}

/** "open" a MappedDevice from an already opened IODevice and a RegisterMap
 * object.
 *  It does not actually open anything, which shows that the "openDev"s should
 * be overloaded
 *  constructors. To be changed in redesign.
 *  This function allows to use BaseDevice as template argument and feed in a
 * dummy
 * or a pcie device.
 */
template <typename T>
void MappedDevice<T>::openDev(ptrdev ioDevice, ptrmapFile registerMap_) {
  pdev = ioDevice;
  registerMap = registerMap_;
}

/**
 *      @brief  Function allows to close the device and release the shared
 *pointers.
 *
 *      Function throws the same exceptions like closeDev from class type
 *      passed as a template parameter.
 */
template <typename T> void MappedDevice<T>::closeDev() {
  checkPointersAreNotNull();
  pdev->closeDev();
}

/**
 *      @brief  Function allows to read data from one register located in
 *              device address space
 *
 *      This is wrapper to standard readReg function defined in libdev
 *library.
 *      Allows to read one register located in device address space. Size of
 *register
 *      depends on type of accessed device e.x. for PCIe device it is equal to
 *      32bit. Function throws the same exceptions like readReg from class
 *type.
 *
 *
 *      @param  regOffset - offset of the register in device address space
 *      @param  data - pointer to area to store data
 *      @param  bar  - number of PCIe bar
 */
template <typename T>
void MappedDevice<T>::readReg(uint32_t regOffset, int32_t *data, uint8_t bar) const {
  checkPointersAreNotNull();
  pdev->readReg(regOffset, data, bar);
}

/**
 *      @brief  Function allows to write data to one register located in
 *              device address space
 *
 *      This is wrapper to standard writeReg function defined in libdev
 *library.
 *      Allows to write one register located in device address space. Size of
 *register
 *      depends on type of accessed device e.x. for PCIe device it is equal to
 *      32bit. Function throws the same exceptions like writeReg from class
 *type.
 *
 *      @param  regOffset - offset of the register in device address space
 *      @param  data - pointer to data to write
 *      @param  bar  - number of PCIe bar
 */
template <typename T>
void MappedDevice<T>::writeReg(uint32_t regOffset, int32_t data, uint8_t bar) {
  checkPointersAreNotNull();
  pdev->writeReg(regOffset, data, bar);
}

/**
 *      @brief  Function allows to read data from several registers located in
 *              device address space
 *
 *      This is wrapper to standard readArea function defined in libdev
 *library.
 *      Allows to read several registers located in device address space.
 *      Function throws the same exceptions like readArea from class type.
 *
 *
 *      @param  regOffset - offset of the register in device address space
 *      @param  data - pointer to area to store data
 *      @param  size - number of bytes to read from device
 *      @param  bar  - number of PCIe bar
 */
template <typename T>
void MappedDevice<T>::readArea(uint32_t regOffset, int32_t *data, size_t size,
                         uint8_t bar) const {
  checkPointersAreNotNull();
  pdev->readArea(regOffset, data, size, bar);
}

template <typename T>
void MappedDevice<T>::writeArea(uint32_t regOffset, int32_t const *data, size_t size,
                          uint8_t bar) {
  checkPointersAreNotNull();
  pdev->writeArea(regOffset, data, size, bar);
}

template <typename T>
void MappedDevice<T>::readDMA(uint32_t regOffset, int32_t *data, size_t size,
                        uint8_t bar) const {
  checkPointersAreNotNull();
  pdev->readDMA(regOffset, data, size, bar);
}

template <typename T>
void MappedDevice<T>::writeDMA(uint32_t regOffset, int32_t const *data, size_t size,
                         uint8_t bar) {
  checkPointersAreNotNull();
  pdev->writeDMA(regOffset, data, size, bar);
}

template <typename T>
void MappedDevice<T>::readDeviceInfo(std::string *devInfo) const {
  checkPointersAreNotNull();
  pdev->readDeviceInfo(devInfo);
}

template <typename T>
MappedDevice<T>::RegisterAccessor::RegisterAccessor(const std::string & /*_regName*/,
                                              const mapFile::mapElem &_me,
                                              ptrdev _pdev)
    : me(_me),
      pdev(_pdev),
      _fixedPointConverter(_me.reg_width, _me.reg_frac_bits, _me.reg_signed) {}

template <typename T>
void MappedDevice<T>::RegisterAccessor::checkRegister(const mapFile::mapElem &me,
                                                size_t dataSize,
                                                uint32_t addRegOffset,
                                                uint32_t &retDataSize,
                                                uint32_t &retRegOff) {
  if (addRegOffset % 4) {
    throw MappedDeviceException("Register offset must be divisible by 4",
                   MappedDeviceException::EX_WRONG_PARAMETER);
  }
  if (dataSize) {
    if (dataSize % 4) {
      throw MappedDeviceException("Data size must be divisible by 4",
      		MappedDeviceException::EX_WRONG_PARAMETER);
    }
    if (dataSize > me.reg_size - addRegOffset) {
      throw MappedDeviceException("Data size exceed register size",
      		MappedDeviceException::EX_WRONG_PARAMETER);
    }
    retDataSize = dataSize;
  } else {
    retDataSize = me.reg_size;
  }
  retRegOff = me.reg_address + addRegOffset;
}

template <typename T>
void MappedDevice<T>::RegisterAccessor::readReg(int32_t *data, size_t dataSize,
                                          uint32_t addRegOffset) const {
  uint32_t retDataSize;
  uint32_t retRegOff;
  checkRegister(me, dataSize, addRegOffset, retDataSize, retRegOff);
  pdev->readArea(retRegOff, data, retDataSize, me.reg_bar);
}

template <typename T>
void MappedDevice<T>::RegisterAccessor::writeReg(int32_t const *data, size_t dataSize,
                                           uint32_t addRegOffset) {
  uint32_t retDataSize;
  uint32_t retRegOff;
  checkRegister(me, dataSize, addRegOffset, retDataSize, retRegOff);
  pdev->writeArea(retRegOff, data, retDataSize, me.reg_bar);
}

template <typename T>
void MappedDevice<T>::RegisterAccessor::readDMA(int32_t *data, size_t dataSize,
                                          uint32_t addRegOffset) const {
  uint32_t retDataSize;
  uint32_t retRegOff;
  checkRegister(me, dataSize, addRegOffset, retDataSize, retRegOff);
  if (me.reg_bar != 0xD) {
    throw MappedDeviceException("Cannot read data from register \"" + me.reg_name +
                       "\" through DMA",
											 MappedDeviceException::EX_WRONG_PARAMETER);
  }
  pdev->readDMA(retRegOff, data, retDataSize, me.reg_bar);
}

template <typename T>
void MappedDevice<T>::RegisterAccessor::writeDMA(int32_t const *data, size_t dataSize,
                                           uint32_t addRegOffset) {
  uint32_t retDataSize;
  uint32_t retRegOff;
  checkRegister(me, dataSize, addRegOffset, retDataSize, retRegOff);
  if (me.reg_bar != 0xD) {
    throw MappedDeviceException("Cannot read data from register \"" + me.reg_name +
                       "\" through DMA",
											 MappedDeviceException::EX_WRONG_PARAMETER);
  }
  pdev->writeDMA(retRegOff, data, retDataSize, me.reg_bar);
}

template <typename T>
mapFile::mapElem const &MappedDevice<T>::RegisterAccessor::getRegisterInfo() const {
  return me; // me is the mapElement
}

template <typename T>
FixedPointConverter const &MappedDevice<T>::RegisterAccessor::getFixedPointConverter()
    const {
  return _fixedPointConverter;
}


template <typename T> void MappedDevice<T>::checkPointersAreNotNull() const {
  if ((pdev == false) || (registerMap == false)) {
    throw MappedDeviceException("MappedDevice has not been opened correctly",
                  MappedDeviceException::EX_NOT_OPENED);
  }
}

template <typename T>
template <typename ConvertedDataType>
ConvertedDataType MappedDevice<T>::RegisterAccessor::read() const {
  ConvertedDataType t;
  read(&t);
  return t;
}

template <typename T>
template <typename ConvertedDataType>
void MappedDevice<T>::RegisterAccessor::write(ConvertedDataType const *convertedData,
                                        size_t nWords,
                                        uint32_t wordOffsetInRegister) {
  // Check that nWords is not 0. The readReg command would read the whole
  // register, which
  // will the raw buffer size of 0.
  if (nWords == 0) {
    return;
  }

  std::vector<int32_t> rawDataBuffer(nWords);
  for (size_t i = 0; i < nWords; ++i) {
    rawDataBuffer[i] = _fixedPointConverter.toFixedPoint(convertedData[i]);
  }
  writeReg(&(rawDataBuffer[0]), nWords * sizeof(int32_t),
           wordOffsetInRegister * sizeof(int32_t));
}

template <typename T>
template <typename ConvertedDataType>
void MappedDevice<T>::RegisterAccessor::write(
    ConvertedDataType const &convertedData) {
  write(&convertedData, 1);
}

template <typename T>
template <typename customClass>
boost::shared_ptr<customClass> MappedDevice<T>::getCustomAccessor(
    const std::string &dataRegionName, const std::string &module) const {
  return (
      customClass::createInstance(dataRegionName, module, pdev, registerMap));
}


template <typename T>
inline mtca4u::MappedDevice<T>::MappedDevice(boost::shared_ptr<BaseDevice> baseDevice,
                                 const std::string &mapFile)
    : pdev(baseDevice),
      registerMap(mtca4u::mapFileParser().parse(mapFile)) {}

} // namespace mtca4u

#endif /* MTCA4U_MAPPEDDEVICE_H */
