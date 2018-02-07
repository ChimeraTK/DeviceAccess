/**
 *      @file           Exception.h
 *      @author         Adam Piotrowski <adam.piotrowski@desy.de>
 *      @version        1.0
 *      @brief          Provides base class for exception handling                
 */
#ifndef MTCA4U_EXCEPTION_H
#define MTCA4U_EXCEPTION_H

#include <exception>
#include <string>

namespace ChimeraTK {

  /**
   *      @brief  Provides base class for exception handling .
   *
   *      Stores exception ID and exception description in the form of text string.
   *
   */
  class Exception : public std::exception {
    protected:
      std::string         exMessage;      /**< exception description*/
      unsigned int        exID;           /**< exception ID*/
    public:
      /**
       * @brief Class constructor
       *
       * @param _exMessage exception description string
       * @param _exID exception ID
       */
      Exception(const std::string &_exMessage, unsigned int _exID);
      /**
       * @brief Accessor. Returns exception description string
       *
       * @return exception description string
       */
      virtual const char* what() const throw();
      /**
       * @brief Accessor. Returns exception ID
       *
       * @return exception ID
       */
      virtual unsigned int getID() const;
      /**
       * Class destructor
       */
      virtual ~Exception() throw();
    private:

  };

}//namespace ChimeraTK

#endif  /* MTCA4U_EXCEPTION_H */

