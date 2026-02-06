// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <boost/shared_ptr.hpp>

#include <cstdint>
#include <iostream>
#include <list>
#include <string>
#include <vector>

namespace ChimeraTK {

  /**
   *      @brief  Provides container to store information about devices described
   * in DMAP file.
   *
   *      Stores detailed information about all devices described in DMAP file.
   *      Provides functionality like searching for detailed information about
   *      device and checking for DMAP file correctness. Does not perform DMAP
   * file parsing
   *
   */
  class DeviceInfoMap {
   public:
    /**
     * @brief  Stores information about one device
     */
    class DeviceInfo {
     public:
      std::string deviceName;      /**< logical name of the device*/
      std::string uri;             /**< uri which describes the device (or name of the device
                                      file in /dev in backward compatibility mode)*/
      std::string mapFileName;     /**< name of the MAP file storing information about
                                      PCIe registers mapping*/
      std::string dmapFileName;    /**< name of the DMAP file*/
      uint32_t dmapFileLineNumber; /**< line number in DMAP file storing listed
                                      above information*/

      /**
       * Default class constructor
       */
      DeviceInfo();

      /** Convenience function to extract the device file name and the map file
       * name as one object (a pair). This is all the information needed to open a
       * Device opject. As std::pair and std::string are standard objects no
       * dependency between DeviceInfoMap and the Device object is introduced, in
       * contrast to passing a DeviceInfo to Device. The function name is a bit
       * lengthy to avoid confusion between device name (logical name) and device
       * file name (name of the device in the /dev directory). The latter is the
       * .first argument of the pair.
       */
      [[nodiscard]] std::pair<std::string, std::string> getDeviceFileAndMapFileName() const;

      friend std::ostream& operator<<(std::ostream& os, const DeviceInfo& deviceInfo);
    };

    using iterator = std::vector<DeviceInfo>::iterator;
    using const_iterator = std::vector<DeviceInfo>::const_iterator;
    /**
     * @brief  Stores information about errors and warnings
     *
     * Stores information about all errors and warnings detected during DMAP file
     * correctness check
     */
    class ErrorList {
      friend class DeviceInfoMap;

     public:
      /**
       * @brief  Stores detailed information about one error or warning
       *
       * Stores detailed information about one error or warnings detected during
       * DMAP file correctness check
       */
      class ErrorElem {
       public:
        /**
         * @brief  Defines available types of detected problems
         */
        enum DMAP_FILE_ERR {
          NONUNIQUE_DEVICE_NAME /**< Names of two devices are the same - treated
                                   as critical error */
        };

        /**
         * @brief  Defines available classes of detected problems
         *
         * Posibble values are ERROR or WARNING - used if user wants to limit
         * number of reported problems only to critical errors or wants to report
         * all detected problems (errors and warnings)
         */
        enum class TYPE {
          ERROR,  /**< Critical error was detected */
          WARNING /**< Non-critical error was detected */
        };
        using TYPE::ERROR;
        using TYPE::WARNING;
        DeviceInfoMap::DeviceInfo _errorDevice1; /**< Detailed information about first device that
                                                    generate error or warning */
        DeviceInfoMap::DeviceInfo _errorDevice2; /**< Detailed information about second device that
                                                    generate error or warning */
        DMAP_FILE_ERR _errorType;                /**< Type of detected problem */
        TYPE _type;                              /**< Class of detected problem - ERROR or WARNING*/

        /**
         * Creates obiect that describe one detected error or warning
         *
         * @param infoType type of detected problem - ERROR or WARNING
         * @param errorType detailed type of detected problem
         * @param device1 detailed information about first device that generate
         * problem
         * @param device2 detailed information about second device that generate
         * problem
         */
        ErrorElem(TYPE infoType, DMAP_FILE_ERR errorType, const DeviceInfoMap::DeviceInfo& device1,
            const DeviceInfoMap::DeviceInfo& device2);
        friend std::ostream& operator<<(std::ostream& os, const TYPE& me);
        friend std::ostream& operator<<(std::ostream& os, const ErrorElem& me);
      };
      std::list<ErrorElem> _errors; /**< Lists of errors or warnings detected
                                       during MAP file correctness checking*/

      friend std::ostream& operator<<(std::ostream& os, const ErrorList& me);

     private:
      /**
       * Delete all elements from error list
       */
      void clear();
      /**
       * Insert new error on error list
       * @param elem object describing detected error or warning
       */
      void insert(const ErrorElem& elem);
    };
    /**
     * @brief Checks logical correctness of DMAP file.
     *
     * Checks if names in DMAP file are unique. Errors are not reported
     * if two devices with the same name have the same parameters. Checks only
     * syntactic correctness of data stored in DMAP file. Syntax and lexical
     * analizys are performed by DMAP file parser.
     *
     * @param err list of detected errors
     * @param level level of checking - if ERROR is selected only errors will be
     * reported, if WARNING is selected errors and warning will be reported
     * @return false if error or warning was detected, otherwise true
     *
     */
    bool check(ErrorList& err, ErrorList::ErrorElem::TYPE level);

    friend std::ostream& operator<<(std::ostream& os, const DeviceInfoMap& deviceInfoMap);
    /**
     * @brief Returns information about specified device
     *
     * @throw exDmapFile [exLibMap::EX_NO_DEVICE_IN_DMAP_FILE] - no device with
     * specified name
     * @param deviceName name of the device
     * @param value detailed information about device taken from DMAP file
     *
     */
    void getDeviceInfo(const std::string& deviceName, DeviceInfo& value);
    /**
     * @brief Returns number of records in DMAP file
     *
     * @return number of records in DMAP file
     */
    size_t getSize();
    /**
     * @brief Return iterator to first device described in DMAP file
     *
     * @return iterator to first element in DMAP file
     *
     */
    iterator begin();
    [[nodiscard]] const_iterator begin() const;
    /**
     * @brief Return iterator to element after last one in DMAP file
     *
     * @return iterator to element after last one in DMAP file
     *
     */
    iterator end();
    [[nodiscard]] const_iterator end() const;

    /** You can define shared libraries with Backend plugins in the DMAP file.
     */
    std::vector<std::string> getPluginLibraries();

    /** Add the name of a library to the list.
     */
    void addPluginLibrary(const std::string& soFile);

   protected:
    std::vector<DeviceInfo> _deviceInfoElements; /**< vector storing parsed contents of DMAP file*/
    std::string _dmapFileName;                   /**< name of DMAP file*/
    std::vector<std::string> _pluginLibraries;   ///< Names of the so files with the plugins

   public:
    /**
     * @brief Constructor
     *
     * Initialize DMAP file name stored into object but does not perform DMAP file
     * parsing
     *
     * @param fileName name of DMAP file
     */
    explicit DeviceInfoMap(std::string fileName);
    /**
     * @brief Insert new element read from DMAP file
     * @param elem element describing detailes of one device taken from DMAP file
     */
    void insert(const DeviceInfo& elem);
  };
  /**
   * @typedef DeviceInfoMapPointer
   * Introduce specialisation of shared_pointer template for pointers to
   * RegisterInfoMap object as a DeviceInfoMapPointer
   */
  using DeviceInfoMapPointer = boost::shared_ptr<DeviceInfoMap>;

} // namespace ChimeraTK
