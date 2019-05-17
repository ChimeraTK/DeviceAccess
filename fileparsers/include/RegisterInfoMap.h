/**
 *      @brief          Provides storage object for registers description taken
 * from MAP file
 */
#ifndef CHIMERA_TK_MAP_FILE_H
#define CHIMERA_TK_MAP_FILE_H

#include <boost/shared_ptr.hpp>
#include <iostream>
#include <list>
#include <stdint.h>
#include <string>
#include <vector>

#include "RegisterCatalogue.h"
#include "RegisterInfo.h"

namespace ChimeraTK {

  /**
   *      @brief  Provides container to store information about registers
   * described in MAP file.
   *
   *      Stores detailed information about all registers described in MAP file.
   *      Provides functionality like searching for detailed information about
   *      registers and checking for MAP file correctness. Does not perform MAP
   * file parsing
   *
   */
  class RegisterInfoMap {
   public:
    /**
     * @brief  Stores information about meta data stored in MAP file
     *
     * Stores metadata, additional attribute in form of pair name-value associeted
     * with map file
     */
    class MetaData {
     public:
      std::string name;  /**< Name of metadata attribute */
      std::string value; /**< Value of metadata attribute */
      friend std::ostream& operator<<(std::ostream& os, const MetaData& me);

      /// Convenience constructor which sets all data members. They all have
      /// default values, so this also acts as default constructor.
      MetaData(std::string const& the_name = std::string(), // an empty string
          std::string const& the_value = std::string());    // another empty string
    };

    /**
     * @brief  Stores information about one PCIe register
     *
     * Stores detailed information about PCIe register and location of its
     * description in MAP file.
     */
    class RegisterInfo : public ChimeraTK::RegisterInfo {
     public:
      enum Access { READ = 1 << 0, WRITE = 1 << 1, READWRITE = READ | WRITE };
      /** Enum descibing the data interpretation:
       *  \li Fixed point (includes integer = 0 fractional bits)
       *  \li IEEE754 floating point
       *  \li ASCII ascii characters
       *  \li VOID no data content, just trigger events (push type) FIXME:
       * Currently implicit by 0 bits width
       */
      enum Type { FIXED_POINT, IEEE754, ASCII };

      RegisterPath getRegisterName() const override {
        RegisterPath path = RegisterPath(module) / name;
        path.setAltSeparator(".");
        return path;
      }

      unsigned int getNumberOfElements() const override { return nElements; }

      unsigned int getNumberOfChannels() const override {
        if(is2DMultiplexed) return nChannels;
        return 1;
      }

      unsigned int getNumberOfDimensions() const override {
        if(is2DMultiplexed) return 2;
        if(nElements > 1) return 1;
        return 0;
      }

      const DataDescriptor& getDataDescriptor() const override { return dataDescriptor; }

      bool isReadable() const override { return (registerAccess & Access::READ) != 0; }

      bool isWriteable() const override { return (registerAccess & Access::WRITE) != 0; }

      AccessModeFlags getSupportedAccessModes() const override { return {AccessMode::raw}; }

      std::string name;        /**< Name of register */
      uint32_t nElements;      /**< Number of elements in register */
      uint32_t nChannels;      /**< Number of channels/sequences */
      bool is2DMultiplexed;    /**< Flag if register is a 2D multiplexed
                                        register (otherwise it is 1D or scalar) */
      uint32_t address;        /**< Relative address in bytes from beginning  of
                                        the bar(Base Address Range)*/
      uint32_t nBytes;         /**< Size of register expressed in bytes */
      uint32_t bar;            /**< Number of bar with register */
      uint32_t width;          /**< Number of significant bits in the register */
      int32_t nFractionalBits; /**< Number of fractional bits */
      bool signedFlag;         /**< Signed/Unsigned flag */
      std::string module;      /**< Name of the module this register is in*/
      Access registerAccess;   /**< Data access direction: Read, write or read
                                        and write */
      Type dataType;           /**< Data type (fixpoint, floating point)*/

      friend std::ostream& operator<<(std::ostream& os, const RegisterInfo& registerInfo);

      /// Constructor to set all data members. They all have default values, so
      /// this also acts as default constructor.
      RegisterInfo(std::string const& name_ = std::string(), // an empty string
          uint32_t nElements_ = 0, uint32_t address_ = 0, uint32_t nBytes_ = 0, uint32_t bar_ = 0, uint32_t width_ = 32,
          int32_t nFractionalBits_ = 0, bool signedFlag_ = true, std::string const& module_ = std::string(),
          uint32_t nChannels_ = 1, bool is2DMultiplexed_ = false, Access dataAccess_ = Access::READWRITE,
          Type dataType_ = Type::FIXED_POINT);

      // Copy assignment
      RegisterInfo& operator=(const RegisterInfo& other) {
        // Copy assignment is implemented to in-place destroy and then in-place
        // copy-construct the object. This is needed since all our members are
        // const.
        this->~RegisterInfo();
        new(this) RegisterInfo(other);
        return *this;
      }

     protected:
      DataDescriptor dataDescriptor;
    };

    /**
     * @brief  Stores information about errors and warnings
     *
     * Stores information about all errors and warnings detected during MAP file
     * correctness check
     */
    class ErrorList {
      friend class RegisterInfoMap;
      friend class DMapFilesParser;

     public:
      /**
       * @brief  Stores detailed information about one error or warning
       *
       * Stores detailed information about one error or warnings detected during
       * MAP file correctness check
       */
      class ErrorElem {
       public:
        /**
         * @brief  Defines available types of detected problems
         */
        typedef enum {
          NONUNIQUE_REGISTER_NAME, /**< Names of two registers are the same -
                                      treated as critical error */
          WRONG_REGISTER_ADDRESSES /**< Address of register can be incorrect -
                                      treated as warning */
        } MAP_FILE_ERR;

        /**
         * @brief  Defines available classes of detected problems
         *
         * Posibble values are ERROR or WARNING - used if user wants to limit
         * number of reported problems only to critical errors or wants to report
         * all detected problems (errors and warnings)
         */
        typedef enum {
          ERROR,  /**< Critical error was detected */
          WARNING /**< Non-critical error was detected */
        } TYPE;
        RegisterInfo _errorRegister1; /**< Detailed information about first register that
                                         generate error or warning */
        RegisterInfo _errorRegister2; /**< Detailed information about second register that
                                         generate error or warning */
        std::string _errorFileName;   /**< Name of the MAP file with detected error
                                         or warning*/
        MAP_FILE_ERR _errorType;      /**< Type of detected problem */
        TYPE _type;                   /**< Class of detected problem - ERROR or WARNING*/
        /**
         * Creates obiect that describe one detected error or warning
         *
         * @param info_type type of detected problem - ERROR or WARNING
         * @param e_type detailed type of detected problem
         * @param reg_1 detailed information about first register that generate
         * problem
         * @param reg_2 detailed information about second register that generate
         * problem
         * @param file_name MAP file name
         */
        ErrorElem(TYPE info_type, MAP_FILE_ERR e_type, const RegisterInfo& reg_1, const RegisterInfo& reg_2,
            const std::string& file_name);
        friend std::ostream& operator<<(std::ostream& os, const TYPE& me);
        friend std::ostream& operator<<(std::ostream& os, const ErrorElem& me);
      };
      std::list<ErrorElem> errors; /**< Lists of errors or warnings detected
                                      during MAP file correctness checking*/
      friend std::ostream& operator<<(std::ostream& os, const ErrorList& me);

     private:
      void clear();
      void insert(const ErrorElem& elem);
    };
    friend std::ostream& operator<<(std::ostream& os, const RegisterInfoMap& registerInfoMap);

    /**
     * @brief Returns detailed information about selected register
     *
     * Returns RegisterInfo structure that describe selected register. If
     * specified by name register cannot be found in MAP file function throws
     * exception exMapFile with type exLibMap::EX_NO_REGISTER_IN_MAP_FILE.
     *
     * @throw exMapFile (exLibMap::EX_NO_REGISTER_IN_MAP_FILE] - no register with
     * specified name
     * @param reg_name Register name
     * @param value The return value: Detailed information about register
     * @param reg_module \c optional Module in which the register is. Empty in
     * case of simple map files.
     *
     */
    void getRegisterInfo(
        const std::string& reg_name, RegisterInfo& value, const std::string& reg_module = std::string()) const;
    /**
     * @brief Returns detailed information about selected register
     *
     * @deprecated use iterators instead of getRegisterInfo and getMapFileSize
     * @throw exMapFile [exLibMap::EX_NO_REGISTER_IN_MAP_FILE] - no register with
     * specified number
     * @param reg_nr register number
     * @param value detailed information about register
     *
     */
    void getRegisterInfo(int reg_nr, RegisterInfo& value) const;
    /**
     * @brief Returns value of specified metadata
     *
     * Returns value associated with specified by name metadata
     *
     * @throw exMapFile [exLibMap::EX_NO_METADATA_IN_MAP_FILE] - no metadata with
     * specified name
     * @param metaDataName  name of metadata
     * @param metaDataValue value associated with metadata
     *
     */
    void getMetaData(const std::string& metaDataName, std::string& metaDataValue) const;
    /**
     * @brief Checks logical correctness of MAP file.
     *
     * Checks if names in MAP file are unique and if register addresses overlappe.
     * Errors are not reported if two registers with the same name have the same
     * parameters. Checks only syntactic correctness of data stored in MAP file.
     * Syntax and lexical analizys are performed by MAP file parser.
     *
     * @param err list of detected errors
     * @param level level of checking - if ERROE is selected only errors will be
     * reported, if WARNING is selected errors and warning will be reported
     * @return false if error or warning was detected, otherwise true
     *
     */
    bool check(ErrorList& err, ErrorList::ErrorElem::TYPE level);
    /**
     * @brief Returns name of MAP file check
     *
     * @return name of MAP file
     *
     */
    const std::string& getMapFileName() const;
    /**
     * @brief Returns number of register described in MAP file
     *
     * @return number of registers in MAP file
     */
    size_t getMapFileSize() const;

    class const_iterator {
     public:
      const_iterator() {}

      const_iterator(RegisterCatalogue::const_iterator theIterator_) : theIterator(theIterator_) {}

      const_iterator& operator++() { // ++it
        ++theIterator;
        return *this;
      }
      const_iterator operator++(int) { // it++
        const_iterator temp(*this);
        ++theIterator;
        return temp;
      }
      const_iterator& operator--() { // --it
        --theIterator;
        return *this;
      }
      const_iterator operator--(int) { // it--
        const_iterator temp(*this);
        --theIterator;
        return temp;
      }
      const RegisterInfoMap::RegisterInfo& operator*() {
        return *static_cast<const RegisterInfoMap::RegisterInfo*>(theIterator.get().get());
      }
      boost::shared_ptr<const RegisterInfoMap::RegisterInfo> operator->() {
        return boost::static_pointer_cast<const RegisterInfoMap::RegisterInfo>(theIterator.get());
      }
      bool operator==(const const_iterator& rightHandSide) const { return rightHandSide.theIterator == theIterator; }
      bool operator!=(const const_iterator& rightHandSide) const { return rightHandSide.theIterator != theIterator; }

     protected:
      RegisterCatalogue::const_iterator theIterator;
    };

    class iterator {
     public:
      iterator() {}

      iterator(RegisterCatalogue::iterator theIterator_) : theIterator(theIterator_) {}

      iterator& operator++() { // ++it
        ++theIterator;
        return *this;
      }
      iterator operator++(int) { // it++
        iterator temp(*this);
        ++theIterator;
        return temp;
      }
      iterator& operator--() { // --it
        --theIterator;
        return *this;
      }
      iterator operator--(int) { // it--
        iterator temp(*this);
        --theIterator;
        return temp;
      }
      RegisterInfoMap::RegisterInfo& operator*() {
        return *static_cast<RegisterInfoMap::RegisterInfo*>(theIterator.get().get());
      }
      boost::shared_ptr<RegisterInfoMap::RegisterInfo> operator->() {
        return boost::static_pointer_cast<RegisterInfoMap::RegisterInfo>(theIterator.get());
      }
      operator const_iterator() { return {theIterator}; }
      bool operator==(const iterator& rightHandSide) const { return rightHandSide.theIterator == theIterator; }
      bool operator!=(const iterator& rightHandSide) const { return rightHandSide.theIterator != theIterator; }

     protected:
      RegisterCatalogue::iterator theIterator;
    };

    /**
     * @brief Return iterator to first register described in MAP file
     *
     * @return iterator to first element in MAP file
     *
     */
    iterator begin();
    const_iterator begin() const;
    /**
     * @brief Return iterator to element after last one in MAP file
     *
     * @return iterator to element after last one in MAP file
     *
     */
    iterator end();
    const_iterator end() const;

    /** Get a complete list of RegisterInfo objects (RegisterInfo) for one module.
     *  The registers are in alphabetical order.
     */
    std::list<RegisterInfoMap::RegisterInfo> getRegistersInModule(std::string const& moduleName);

   public:
    /**
     * @brief Constructor
     *
     * Default constructor.
     */
    RegisterInfoMap();
    /**
     * @brief Constructor
     *
     * Initialize MAP file name stored into object but does not perform MAP file
     * parsing
     *
     * @param file_name - MAP file name
     */
    RegisterInfoMap(const std::string& file_name);
    /**
     * @brief Inserts new element describing register into RegisterInfoMap object
     *
     * @param elem reference to object describing register
     */
    void insert(RegisterInfo& elem);
    /**
     * @brief Inserts new element describing metadata into RegisterInfoMap object
     *
     * @param elem reference to metadata information available in file.
     */
    void insert(MetaData& elem);

    /** Return the RegisterCatalogue storing the register information */
    const RegisterCatalogue& getRegisterCatalogue();

   private:
    /** name of MAP file*/
    std::string _mapFileName;

    /** the catalogue storing the map file information */
    RegisterCatalogue _catalogue;
  };
  /**
   * @typedef RegisterInfoMapPointer
   * Introduce specialisation of shared_ptr template for pointers to
   * RegisterInfoMap object as a RegisterInfoMapPointer
   */
  typedef boost::shared_ptr<RegisterInfoMap> RegisterInfoMapPointer;

} // namespace ChimeraTK

#endif /* CHIMERA_TK_MAP_FILE_H */
