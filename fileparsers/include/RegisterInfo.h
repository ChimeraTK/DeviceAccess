/*
 * RegisterInfo.h
 *
 *  Created on: Mar 1, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_REGISTER_INFO_H
#define CHIMERA_TK_REGISTER_INFO_H

#include <iostream>

#include "RegisterInfoPlugin.h"
#include "RegisterPath.h"
#include "ForwardDeclarations.h"

namespace ChimeraTK {

  /** DeviceBackend-independent register description. */
  class RegisterInfo {

    public:

      /** Enum for the fundamental data types. This is only used inside the DataDescriptor class but defined outside to
       *  prevent too long fully qualified names. */
      enum class FundamentalType { numeric, string, boolean, nodata, undefined };
      
      /** Class describing the actual payload data format of a register in an abstract manner. It gives information
       *  about the underlying data type without fully describing it, to prevent a loss of abstraction on the
       *  application level. The returned information always refers to the data type and thus is completely independent
       *  of the current value of the register. */
      class DataDescriptor {

        public:
          
          /** Get the fundamental data type */
          FundamentalType fundamentalType() const;
          
          /** Return whether the data is signed or not. May only be called for numeric data types. */
          bool isSigned() const;
          
          /** Return whether the data is integral or not (e.g. int vs. float). May only be called for numeric data
           *  types. */
          bool isIntegral() const;
          
          /** Return the approximate maximum number of digits (of base 10) needed to represent the value (including
           *  a decimal dot, if not an integral data type, and the sign). May only be called for numeric data types.
           *
           *  This number shall only be used for displaying purposes, e.g. to decide how much space for displaying
           *  the register value should be reserved. Beware that for some data types this might become a really large
           *  number (e.g. 300), which indicates that you need to choose a different representation than just a plain
           *  decimal number. */
          size_t nDigits() const;
          
          /** Approximate maximum number of digits after decimal dot (of base 10) needed to represent the value
           *  (excluding the decimal dot itself). May only be called for non-integral numeric data types.
           *
           *  Just like in case of nDigits(), this number should only be used for displaying purposes. There is no
           *  guarantee that the full precision of the number can be displayed with the given number of digits.
           *  Again beware that this number might be rather large (e.g. 300). */
          size_t nFractionalDigits() const;
          
          /** Default constructor sets fundamental type to "undefined" */
          DataDescriptor();

          /** Constructor setting all members. */
          DataDescriptor(FundamentalType fundamentalType, bool isIntegral=false, bool isSigned=false,
                         size_t nDigits=0, size_t nFractionalDigits=0);
          
        private:

          /** The fundamental data type */
          FundamentalType _fundamentalType;

          /** Numeric types only: is the number integral or not */
          bool _isIntegral;
          
          /** Numeric types only: is the number signed or not */
          bool _isSigned;
          
          /** Numeric types only: approximate maximum number of digits (of base 10) needed to represent the value
           *  (including a decimal dot, if not an integral data type) */
          size_t _nDigits;
          
          /** Non-integer numeric types only: Approximate maximum number of digits after decimal dot (of base 10)
           *  needed to represent the value (excluding the decimal dot itself) */
          size_t _nFractionalDigits;

      };

      /** Virtual destructor */
      virtual ~RegisterInfo() {}

      /** Return full path name of the register (including modules) */
      virtual RegisterPath getRegisterName() const = 0;

      /** Return number of elements per channel */
      virtual unsigned int getNumberOfElements() const = 0;

      /** Return number of channels in register */
      virtual unsigned int getNumberOfChannels() const = 0;

      /** Return number of dimensions of this register */
      virtual unsigned int getNumberOfDimensions() const = 0;

      /** Return desciption of the actual payload data for this register. See the description of DataDescriptor for
       *  more information. */
      virtual const DataDescriptor& getDataDescriptor() const = 0;
      
      /** Iterators for the list of plugins */
      class plugin_iterator {
        public:
          plugin_iterator& operator++() {    // ++it
            ++iterator;
            return *this;
          }
          plugin_iterator operator++(int) { // it++
            plugin_iterator temp(*this);
            ++iterator;
            return temp;
          }
          const RegisterInfoPlugin& operator*() const {
            return *(iterator->get());
          }
          const RegisterInfoPlugin* operator->() const {
            return iterator->get();
          }
          const boost::shared_ptr<RegisterInfoPlugin>& getPointer() const {
            return *(iterator);
          }
          bool operator==(const plugin_iterator &rightHandSide) {
            return rightHandSide.iterator == iterator;
          }
          bool operator!=(const plugin_iterator &rightHandSide) {
            return rightHandSide.iterator != iterator;
          }
        protected:
          std::vector< boost::shared_ptr<RegisterInfoPlugin> >::const_iterator iterator;
          friend class RegisterInfo;
      };
      
      /** Return iterators for the list of plugins */
      plugin_iterator plugins_begin() const {
        plugin_iterator i;
        i.iterator = pluginList.cbegin();
        return i;
      }
      plugin_iterator plugins_end() const {
        plugin_iterator i;
        i.iterator = pluginList.cend();
        return i;
      }

    protected:

      /** list of plugins */
      std::vector< boost::shared_ptr<RegisterInfoPlugin> > pluginList;

  };

  /*******************************************************************************************************************/
  
  inline RegisterInfo::FundamentalType RegisterInfo::DataDescriptor::fundamentalType() const {
    return _fundamentalType;
  }
  
  /*******************************************************************************************************************/

  inline bool RegisterInfo::DataDescriptor::isSigned() const {
    assert(_fundamentalType == FundamentalType::numeric);
    return _isSigned;
  }
  
  /*******************************************************************************************************************/

  inline bool RegisterInfo::DataDescriptor::isIntegral() const {
    assert(_fundamentalType == FundamentalType::numeric);
    return _isIntegral;
  }
  
  /*******************************************************************************************************************/

  inline size_t RegisterInfo::DataDescriptor::nDigits() const {
    assert(_fundamentalType == FundamentalType::numeric);
    return _nDigits;
  }
  
  /*******************************************************************************************************************/

  inline size_t RegisterInfo::DataDescriptor::nFractionalDigits() const {
    assert(_fundamentalType == FundamentalType::numeric && !_isIntegral);
    return _nFractionalDigits;
  }
  
  /*******************************************************************************************************************/

  inline RegisterInfo::DataDescriptor::DataDescriptor()
  : _fundamentalType(FundamentalType::undefined)
  {}

  /*******************************************************************************************************************/

  inline RegisterInfo::DataDescriptor::DataDescriptor(FundamentalType fundamentalType_, bool isIntegral_, bool isSigned_,
                                                      size_t nDigits_, size_t nFractionalDigits_)
  : _fundamentalType(fundamentalType_),
    _isIntegral(isIntegral_),
    _isSigned(isSigned_),
    _nDigits(nDigits_),
    _nFractionalDigits(nFractionalDigits_)
  {}
  
} /* namespace ChimeraTK */

#endif /* CHIMERA_TK_REGISTER_INFO_H */
