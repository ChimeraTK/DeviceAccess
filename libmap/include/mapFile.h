/**
 *      @file           mapFile.h
 *      @author         Adam Piotrowski <adam.piotrowski@desy.de>
 *      @version        1.0
 *      @brief          Provides storage object for registers description taken from MAP file                
 */
#ifndef MTCA4U_MAP_FILE_H
#define	MTCA4U_MAP_FILE_H

#include <stdint.h>
#include <string>
#include <vector>
#include <list>
#include <iostream>
#include <boost/shared_ptr.hpp>

namespace mtca4u{

/**
 *      @brief  Provides container to store information about registers described in MAP file. 
 *      
 *      Stores detailed information about all registers described in MAP file. 
 *      Provides functionality like searching for detailed information about 
 *      registers and checking for MAP file correctness. Does not perform MAP file parsing
 *
 */
class mapFile {

public:    
    /**
     * @brief  Stores information about meta data stored in MAP file
     *       
     * Stores metadata, additional attribute in form of pair name-value associeted 
     * with map file 
     */
    class metaData {
    public:
        std::string name; /**< Name of metadata attribute */
        std::string value; /**< Value of metadata attribute */
        friend std::ostream& operator<<(std::ostream &os, const metaData& me);

	/// Convenience constructor which sets all data members. They all have default values, so this 
	/// also acts as default constructor.
	metaData(std::string const & the_name = std::string(), // an empty string
		 std::string const & the_value = std::string()); // another empty string
    };

    /**
     * @brief  Stores information about one PCIe register
     *       
     * Stores detailed information about PCIe register and location of its description in MAP file. 
     */
    class mapElem {
    public:
        std::string reg_name; /**< Name of register */
        uint32_t reg_elem_nr; /**< Number of elements in register */
        uint32_t reg_address; /**< Offset in bytes from begining of PCIe bar */
        uint32_t reg_size; /**< Size of register expressed in bytes */
        uint32_t reg_bar; /**< Number of bar with register */
        uint32_t reg_width; /**< Number of significant bits in the register */
        int32_t  reg_frac_bits; /**< Number of fractional bits */
        bool     reg_signed; /**< Signed/Unsigned flag */
        uint32_t line_nr; /**< Number of line with description of register in MAP file */
        friend std::ostream& operator<<(std::ostream &os, const mapElem& me);
  
	/// Convenience constructor which sets all data members. They all have default values, so this 
	/// also acts as default constructor.
	mapElem(std::string const & the_reg_name = std::string(), // an empty string
		uint32_t the_reg_elem_nr = 0,
		uint32_t the_reg_address = 0,
		uint32_t the_reg_size = 0,
		uint32_t the_reg_bar = 0,
    uint32_t the_reg_width = 32,
    int32_t  the_reg_frac_bits = 0,
    bool     the_reg_signed = true,
		uint32_t the_line_nr = 0);
    };
    typedef std::vector<mapElem>::iterator iterator; 
    typedef std::vector<mapElem>::const_iterator const_iterator;
    /**
     * @brief  Stores information about errors and warnings
     *       
     * Stores information about all errors and warnings detected during MAP file correctness check
     */
    class errorList {
        friend class mapFile;
        friend class dmapFilesParser;
    public:

        /**
         * @brief  Stores detailed information about one error or warning
         *       
         * Stores detailed information about one error or warnings detected during MAP file correctness check
         */
        class errorElem {
        public:

            /**
             * @brief  Defines available types of detected problems
             */
            typedef enum {
                NONUNIQUE_REGISTER_NAME, /**< Names of two registers are the same - treated as critical error */
                WRONG_REGISTER_ADDRESSES /**< Address of register can be incorrect - treated as warning */
            } MAP_FILE_ERR;

            /**
             * @brief  Defines available classes of detected problems
             * 
             * Posibble values are ERROR or WARNING - used if user wants to limit number of reported 
             * problems only to critical errors or wants to report all detected problems (errors and warnings)
             */
            typedef enum {
                ERROR, /**< Critical error was detected */
                WARNING /**< Non-critical error was detected */
            } TYPE;
            mapElem err_reg_1; /**< Detailed information about first register that generate error or warning */
            mapElem err_reg_2; /**< Detailed information about second register that generate error or warning */
            std::string err_file_name; /**< Name of the MAP file with detected error or warning*/
            MAP_FILE_ERR err_type; /**< Type of detected problem */
            TYPE type; /**< Class of detected problem - ERROR or WARNING*/
            /**
             * Creates obiect that describe one detected error or warning
             * 
             * @param info_type type of detected problem - ERROR or WARNING
             * @param e_type detailed type of detected problem  
             * @param reg_1 detailed information about first register that generate problem
             * @param reg_2 detailed information about second register that generate problem
             * @param file_name MAP file name
             */
            errorElem(TYPE info_type, MAP_FILE_ERR e_type, const mapElem &reg_1, const mapElem &reg_2, const std::string &file_name);
            friend std::ostream& operator<<(std::ostream &os, const TYPE& me);
            friend std::ostream& operator<<(std::ostream &os, const errorElem& me);
        };
        std::list<errorElem> errors; /**< Lists of errors or warnings detected during MAP file correctness checking*/
        friend std::ostream& operator<<(std::ostream &os, const errorList& me);
    private:
        void clear();
        void insert(const errorElem& elem);

    };
    friend std::ostream& operator<<(std::ostream &os, const mapFile& me);

    /**
     * @brief Returns detailed information about selected register
     * 
     * Returns mapElem structure that describe selected register. If specified by name register 
     * cannot be found in MAP file function throws exception exMapFile with type exLibMap::EX_NO_REGISTER_IN_MAP_FILE.
     * 
     * @throw exMapFile (exLibMap::EX_NO_REGISTER_IN_MAP_FILE] - no register with specified name
     * @param reg_name register name
     * @param value detailed information about register
     * 
     * @snippet test-libmap.cpp MAP getting info and metadata
     */
    void getRegisterInfo(const std::string& reg_name, mapElem &value) const;
    /**
     * @brief Returns detailed information about selected register
     * 
     * @deprecated use iterators instead of getRegisterInfo and getMapFileSize
     * @throw exMapFile [exLibMap::EX_NO_REGISTER_IN_MAP_FILE] - no register with specified number
     * @param reg_nr register number 
     * @param value detailed information about register
     * 
     */
    void getRegisterInfo(int reg_nr, mapElem &value) const;
    /**
     * @brief Returns value of specified metadata 
     * 
     * Returns value associated with specified by name metadata
     * 
     * @throw exMapFile [exLibMap::EX_NO_METADATA_IN_MAP_FILE] - no metadata with specified name
     * @param metaDataName  name of metadata
     * @param metaDataValue value associated with metadata
     * 
     * @snippet test-libmap.cpp MAP getting info and metadata
     */
    void getMetaData(const std::string &metaDataName, std::string& metaDataValue) const;
    /**
     * @brief Checks logical correctness of MAP file. 
     * 
     * Checks if names in MAP file are unique and if register addresses overlappe. Errors are not reported
     * if two registers with the same name have the same parameters. Checks only syntactic correctness of 
     * data stored in MAP file. Syntax and lexical analizys are performed by MAP file parser. 
     * 
     * @param err list of detected errors
     * @param level level of checking - if ERROE is selected only errors will be reported, if WARNING is selected
     *          errors and warning will be reported 
     * @return false if error or warning was detected, otherwise true
     * 
     * @snippet test-libmap.cpp MAP file correctness checking
     */
    bool check(errorList &err, errorList::errorElem::TYPE level);
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
    /**
     * @brief Return iterator to first register described in MAP file
     * 
     * @return iterator to first element in MAP file
     * 
     * @snippet test-libmap.cpp MAP iterating throught all registers
     */
    iterator begin();
    const_iterator begin() const;
    /**
     * @brief Return iterator to element after last one in MAP file
     * 
     * @return iterator to element after last one in MAP file
     *      
     * @snippet test-libmap.cpp MAP iterating throught all registers
     */
    iterator end();
    const_iterator end() const;

public:
    /**
     * @brief Constructor
     * 
     * Initialize MAP file name stored into object but does not perform MAP file parsing
     * 
     * @param file_name - MAP file name
     */
    mapFile(const std::string &file_name);
    /**
     * @brief Inserts new element describing register into mapFile object
     * 
     * @param elem reference to object describing register
     */
    void insert(mapElem &elem);
    /**
     * @brief Inserts new element describing metadata into mapFile object
     * 
     * @param elem reference to metadata information available in file.
     */
    void insert(metaData &elem);

private:
    std::vector<mapElem> map_file_elems; /**< list of all registers described in MAP file*/
    std::vector<metaData> metadata; /**< list of all metadata detected in MAP file*/
    std::string map_file_name; /**< name of MAP file*/
};
/**
 * @typedef Introduce specialisation of shared_ptr template for pointers to mapFile object as a ptrmapFile
 */
typedef boost::shared_ptr<mapFile> ptrmapFile;


/**
 * @example test-libmap.cpp 
 */

}//namespace mtca4u

#endif	/* MTCA4U_MAP_FILE_H */

