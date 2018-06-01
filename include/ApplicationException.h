/*
 * ApplicationException.h
 *
 *  Created on: Jun 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_APPLICATION_EXCEPTION_H
#define CHIMERATK_APPLICATION_EXCEPTION_H

#include <exception>
#include <string>

namespace ChimeraTK {

  /** IDs for exceptions */
  enum class ApplicationExceptionID {

      /** An illegal parameter has been passed to a function. */
      illegalParameter,

      /** The variable network is not legal, e.g. more than one output accessor is connected to the network. */
      illegalVariableNetwork,

      /** Functionality has been used which is not yet implemented, but will be implemented at a later time. */
      notYetImplemented,

  };

  class ApplicationException : public std::exception {

    public:

      /** returns an explanatory string */
      const char* what() const noexcept { return _what.c_str(); }

      /** returns the ID describing the exception type */
      ApplicationExceptionID getID() const { return _id; }

    protected:

      /** the constructor is protected, create instances of ApplicationExceptionWithID instead! */
      ApplicationException(ApplicationExceptionID id, const std::string& description)
      : _id(id), _what(description)
      {}

      ApplicationExceptionID _id;
      std::string _what;

  };

  template<ApplicationExceptionID id>
  class ApplicationExceptionWithID : public ApplicationException {

    public:

      /** pass the explanatory string returned by what() to the constructor */
      ApplicationExceptionWithID(const std::string& description)
      : ApplicationException(id, description)
      {}
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_APPLICATION_EXCEPTION_H */
