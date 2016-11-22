/**
 *      @file           Exception.h
 *      @author         Adam Piotrowski <adam.piotrowski@desy.de>
 *      @version        1.0
 *      @brief          Provides exception classess for libmap                
 */
#ifndef MTCA4U_MAP_EXCEPTION_H
#define MTCA4U_MAP_EXCEPTION_H

#include <string>

#include "Exception.h"

namespace mtca4u {

  /**
   *      @brief  Provides base class for all exceptions from libmap
   */
  class LibMapException : public Exception {
    public:

      enum {
        EX_CANNOT_OPEN_MAP_FILE, /**< Cannot open MAP file*/
        EX_CANNOT_OPEN_DMAP_FILE, /**< Cannot open DMAP file*/
        EX_FILES_CHECK_ERROR, /**< During correctness checking error or warning was detected*/
        EX_MAP_FILE_PARSE_ERROR, /**< During MAP file parsing error or warning was detected*/
        EX_NO_REGISTER_IN_MAP_FILE, /**< Specified register is not available in MAP file*/
        EX_NO_METADATA_IN_MAP_FILE, /**< Specified metadata is not available in MAP file*/
        EX_NO_DEVICE_IN_DMAP_FILE, /**< Specified device is not available in DMAP file*/
        EX_DMAP_FILE_PARSE_ERROR, /**< During DMAP file parsing error or warning was detected*/
        EX_NO_DMAP_DATA /**< DMAP file is empty or does not exist*/
      };
      /**
       * @brief Class constructor
       *
       * @param _exMessage exception reason expressed as a string
       * @param _exID exception reason expressed as a identifier
       */
      LibMapException(const std::string &_exMessage, unsigned int _exID);

      virtual ~LibMapException() throw ();

      friend std::ostream& operator<<(std::ostream &os, const LibMapException& e);

  };

  /**
   *      @brief  Provides class for exceptions related to MAP file parsing
   */
  class MapFileException : public LibMapException {
    public:
      /**
       * @brief Class constructor
       *
       * @param _exMessage exception reason expressed as a string
       * @param _exID exception reason expressed as a identifier
       */
      MapFileException(const std::string &_exMessage, unsigned int _exID);
  };

  /**
   *      @brief  Provides class for exceptions related to MAP file
   */
  class MapFileParserException : public MapFileException {
    public:
      /**
       * @brief Class constructor
       *
       * @param _exMessage exception reason expressed as a string
       * @param _exID exception reason expressed as a identifier
       */
      MapFileParserException(const std::string &_exMessage, unsigned int _exID);
  };

  /**
   *      @brief  Provides class for exceptions related to DMAP file parsing
   */
  class DMapFileException : public LibMapException {
    public:
      /**
       * @brief Class constructor
       *
       * @param _exMessage exception reason expressed as a string
       * @param _exID exception reason expressed as a identifier
       */
      DMapFileException(const std::string &_exMessage, unsigned int _exID);
  };

  /**
   *      @brief  Provides class for exceptions related to DMAP
   */
  class DMapFileParserException : public DMapFileException {
    public:
      /**
       * @brief Class constructor
       *
       * @param _exMessage exception reason expressed as a string
       * @param _exID exception reason expressed as a identifier
       */
      DMapFileParserException(const std::string &_exMessage, unsigned int _exID);
  };

}//namespace mtca4u

#endif  /* MTCA4U_MAP_EXCEPTION_H */

