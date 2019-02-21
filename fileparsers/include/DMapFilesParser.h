/**
 *      @brief          Provides method to parse DMAP files and MAP files
 * associeted with devices listed in DMAP files. Additionally, provides storage
 * for infomration taken from DMAP and MAP files.
 */
#ifndef CHIMERA_TK_DMAPFILESPARSER_H
#define CHIMERA_TK_DMAPFILESPARSER_H

#include "DMapFileParser.h"
#include "MapFileParser.h"
#include "RegisterInfoMap.h"
#include <stdint.h>
#include <string>

namespace ChimeraTK {

/**
 * @brief  Provides method to parse DMAP files and MAP files associeted with
 *         devices listed in DMAP files
 *
 * Class handles functionality of DMAP and MAP file parsersing.
 */
class DMapFilesParser {
public:
  typedef std::vector<std::pair<DeviceInfoMap::DeviceInfo,
                                RegisterInfoMapPointer>>::iterator iterator;
  typedef std::vector<std::pair<DeviceInfoMap::DeviceInfo,
                                RegisterInfoMapPointer>>::const_iterator
      const_iterator;

private:
  DMapFileParser _dmapFileParser; /**< DMAP file parser*/
  MapFileParser _mapFileParser;   /**< MAP file parser*/
  std::vector<std::pair<DeviceInfoMap::DeviceInfo, RegisterInfoMapPointer>>
      _dmapElements; /**< vector composed of
                        devices and associated
                        pointers to parsed MAP
                        files*/
  std::vector<RegisterInfoMapPointer>
      _mapFiles; /**< vector of parsed MAP files*/
  /**
   * @brief Performs parsing of all DMAP files located in directory passed as a
   * parameter and all MAP files taken from DMAP file and associated with
   * devices .
   *
   * @throw exDmapFileParser [exLibMap::EX_CANNOT_OPEN_DMAP_FILE] if specified
   * DMAP file cannot be opened, exMapFileParser
   * [exLibMap::EX_NO_DATA_IN_DMAP_FILES] if specified DMAP file is empty or
   * exMapFileParser [libmap_ex::EX_FILES_CHECK_ERROR] if logical error im DMAP
   * or MAP file was detected. Additionally it may throw all exceptions form
   * mapFileParser::parse and dmapFileParser::parse
   *
   * @param dir path to directory with DMAP files to parse
   */
  void parse_one_directory(const std::string &dir);

public:
  /**
   * @brief Class constructor
   */
  DMapFilesParser();
  /**
   * @brief Class constructor that perform parsing of all DMAP files located in
   * specified directory
   *
   * @param dir path to directory with DMAP files to parse
   */
  DMapFilesParser(const std::string &dir);
  /**
   * Class destructor
   */
  ~DMapFilesParser();

  /**
   * @brief Performs parsing of one file
   * @param fileName name of file to parse
   *
   */
  void parse_file(const std::string &fileName);
  /**
   * @brief Perform parsing of all DMAP files located in specified directory
   *
   * @param dir path to directory with DMAP files to parse
   *
   */
  void parse_dir(const std::string &dir);
  /**
   * @brief Perform parsing of all DMAP files located in specified directories
   *
   * @param dirs list of pathes to directories with DMAP files to parse
   *
   */
  void parse_dirs(const std::vector<std::string> &dirs);
  /**
   * * @brief Returns detailed information about selected register
   *
   * Returns RegisterInfo structure that describe selected register. If
   * specified by name register cannot be found in MAP file function throws
   * exception exMapFile with type exLibMap::EX_NO_REGISTER_IN_MAP_FILE.
   *
   * @throw exMapFile (exLibMap::EX_NO_REGISTER_IN_MAP_FILE] - no register with
   * specified name
   *
   * @param dev_name name of device
   * @param reg_name name of register
   * @param dev_file name of device file [returned parameter]
   * @param reg_elem_nr number of elements in register [returned parameter]
   * @param reg_offset register offset in PCIe addressing space [returned
   * parameter]
   * @param reg_size total size of register in bytes [returned parameter]
   * @param reg_bar number of PCIe bar [returned parameter]
   */
  void getRegisterInfo(std::string dev_name, const std::string &reg_name,
                       std::string &dev_file, uint32_t &reg_elem_nr,
                       uint32_t &reg_offset, uint32_t &reg_size,
                       uint32_t &reg_bar);

  /**
   * @brief Returns detailed information about selected register
   *
   * Returns RegisterInfo structure that describe selected register. If
   * specified by name register cannot be found in MAP file function throws
   * exception exMapFile with type exLibMap::EX_NO_REGISTER_IN_MAP_FILE.
   *
   * @throw exMapFile (exLibMap::EX_NO_REGISTER_IN_MAP_FILE] - no register with
   * specified name
   * @param dev_name name of device
   * @param reg_name name of register
   * @param dev_file name of device file [returned parameter]
   * @param elem detailed information about register
   *
   */
  void getRegisterInfo(std::string dev_name, const std::string &reg_name,
                       std::string &dev_file,
                       RegisterInfoMap::RegisterInfo &elem);
  /**
   * @brief Returns pointer to parser MAP file asociated with selected device
   *
   * @param dev_name name of device
   * @return pointer to parser MAP file asociated with selected device
   */
  RegisterInfoMapPointer getMapFile(const std::string &dev_name);
  /**
   * @brief Returns number of elements in parsed DMAP files
   *
   * @return number of elements in parsed DMAP files
   */
  uint16_t getdMapFileSize();

  /** Get the dRegisterInfo from the device name.
   *  @deprecated Use the getter function which returns the result instead of
   * passing it by reference.
   */
  void getdMapFileElem(const std::string &devName,
                       DeviceInfoMap::DeviceInfo &dMapFileElem);

  /** Get the dRegisterInfo from the device name.
   *  @attention The reference is only valid as long as the dmapFilesParser
   * object is in scope and no new parse action has been performed. In practice
   * this should not be a limitation. Just use <pre>dmapFile::dRegisterInfo
   * myDeviceInfo = myDmapFilesParser->getdMapFileElem(deviceName); //creates a
   * copy</pre> instead of <pre>dmapFile::dRegisterInfo const & myDeviceInfo =
   * myDmapFilesParser->getdMapFileElem(deviceName); //does not create a
   * copy</pre>
   */
  DeviceInfoMap::DeviceInfo const &getdMapFileElem(const std::string &devName);

  /**
   * @brief Returns detailed information about device specified by number
   *
   * @deprecated use iterators instead of getRegisterInfo and getdMapFileSize
   * @param elem_nr device number
   * @param dMapFileElem detailed information about register specified by number
   */
  void getdMapFileElem(int elem_nr, DeviceInfoMap::DeviceInfo &dMapFileElem);
  /**
   * @brief  Checks correctness of DMAP files and associated MAP files
   *
   * @param dlevel level of DMAP files checking
   * @param mlevel level of MAP files checking
   * @param err list of errors detected in DMAP files
   * @param map_err list of errors detected in MAP files
   * @return true if succeed, otherwise false
   *
   */
  bool check(DeviceInfoMap::ErrorList::ErrorElem::TYPE dlevel,
             RegisterInfoMap::ErrorList::ErrorElem::TYPE mlevel,
             DeviceInfoMap::ErrorList &err,
             RegisterInfoMap::ErrorList &map_err);
  /**
   * @brief Returns iterator to first pair of device and its MAP file described
   * in DMAP file
   *
   * @return iterator to first element in DMAP file
   *
   */
  iterator begin();
  const_iterator begin() const;

  /**
   * @brief Returns iterator to element after last one in DMAP file
   *
   * @return iterator to element after last one in DMAP file
   *
   */
  iterator end();
  const_iterator end() const;

  friend std::ostream &operator<<(std::ostream &os,
                                  const DMapFilesParser &dmfp);

private:
  /**
   * @brief Clean all data structures used to store information about MAP and
   * DMAP files contents
   */
  void cleanAll();
};

} // namespace ChimeraTK

#endif /*  CHIMERA_TK_DMAPFILESPARSER_H */
