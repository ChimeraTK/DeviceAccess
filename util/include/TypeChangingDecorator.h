// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "NDRegisterAccessorDecorator.h"
#include "SupportedUserTypes.h"
#include "TransferElementAbstractor.h"

#include <boost/fusion/include/for_each.hpp>
#include <boost/numeric/conversion/converter.hpp>

namespace ChimeraTK {
  /** There are two types of TypeChanging decorators which do different data
   * conversions from the user data type to the implementation data type.
   *
   *  \li limiting This decorator
   * limits the data to the maximum possible in the target data type (for instance
   * 500 will result in 127 in int8_t and  255 in uint8_t, -200 will be -128 in
   * int8t, 0 in uint8_t). This decorator also does correct rounding from floating
   * point to integer type.
   * \li C_style_conversion This decorator does a direct
   * cast like an assigment in C/C++ does it. For instance 500 (=0x1f4) will
   * result in 0xf4 for an 8 bit integer, which is interpreted as 244 in uint8_t
   * and -12 in int8_t. Digits after the decimal point are cut when converting a
   * floating point value to an integer type. This decorator can be useful to
   * display unsigned integers which use the full dynamic range in a control
   * system which only supports signed data types (the user has to correctly
   * interpret the 'wrong' representation), of for bit fields where it is
   * acceptable to lose the higher bits.
   */
  enum class DecoratorType { limiting, C_style_conversion };

  /** The factory function for type changing decorators.
   *
   *  TypeChanging decorators take a transfer element (usually
   * NDRegisterAccessor<ImplType>) and wraps it in an
   * NDRegisterAccessor<UserType>. It automatically performs the right type
   * conversion (configurable as argument of the factory function). The decorator
   * has it's own buffer of type UserType and synchronises it in the preWrite()
   * and postRead() functions with the implementation. You don't have to care
   * about the implementation type of the transfer element. The factory will
   * automatically create the correct decorator.
   *
   * Note: it is possible to obtain multiple decorators of different types for the same accessor. The user needs to
   * ensure that the preXxx/postXxx transfer functions are properly called for all decorators when required.
   *
   *  @param transferElement The TransferElement to be decorated. It can either be
   * an NDRegisterAccessor (usually the case) or and NDRegisterAccessorAbstractor (but
   * here the user already picks the type he wants).
   *  @param decoratorType The type of decorator you want (see description of
   * DecoratorType)
   */
  template<class UserType>
  boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> getTypeChangingDecorator(
      const boost::shared_ptr<ChimeraTK::TransferElement>& transferElement,
      DecoratorType decoratorType = DecoratorType::limiting);

  template<class UserType>
  boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> getTypeChangingDecorator(
      ChimeraTK::TransferElementAbstractor& transferElement, DecoratorType decoratorType = DecoratorType::limiting) {
    return getTypeChangingDecorator<UserType>(transferElement.getHighLevelImplElement(), decoratorType);
  }

  //====================================================================================================================
  // Everything from here are implementation details. The exact identity of the
  // decorator should not matter to the user, it would break the abstraction.
  //====================================================================================================================

  // Base class which just holds the decorator type. This allows
  class DecoratorTypeHolder {
   public:
    virtual DecoratorType getDecoratorType() const = 0;
  };

  /**
   * Strictly this is not a real decorator. It provides an NDRegisterAccessor of a
   * different template type and accesses the original one as "backend" under the
   * hood.
   *
   * This is the base class which has pure virtual convertAndCopyFromImpl and
   * convertAndCopyToImpl functions. Apart from this it implements all decorating
   * functions which just pass through to the target.
   *
   * This class is not thread-safe and should only be used from a single thread.
   *
   */
  template<class T, class IMPL_T>
  class TypeChangingDecorator : public ChimeraTK::NDRegisterAccessorDecorator<T, IMPL_T>, public DecoratorTypeHolder {
   public:
    TypeChangingDecorator(boost::shared_ptr<ChimeraTK::NDRegisterAccessor<IMPL_T>> const& target) noexcept;

    virtual void convertAndCopyFromImpl() = 0;
    virtual void convertAndCopyToImpl() = 0;
    void doPreRead(ChimeraTK::TransferType type) override { _target->preRead(type); }

    void doPostRead(ChimeraTK::TransferType type, bool hasNewData) override {
      _target->setActiveException(this->_activeException);
      _target->postRead(type, hasNewData);

      // Decorators have to copy meta data even if updataDataBuffer is false
      this->_dataValidity = _target->dataValidity();
      this->_versionNumber = _target->getVersionNumber();

      // If the delegated postRead throws, we don't want to copy into the
      // buffer
      if(hasNewData) {
        convertAndCopyFromImpl();
      }
    }

    void doPreWrite(ChimeraTK::TransferType type, VersionNumber versionNumber) override {
      convertAndCopyToImpl();
      _target->setDataValidity(this->_dataValidity);
      _target->preWrite(type, versionNumber);
    }

    void doPostWrite(ChimeraTK::TransferType type, ChimeraTK::VersionNumber versionNumber) override {
      _target->setActiveException(this->_activeException);
      _target->postWrite(type, versionNumber);
    }

    bool mayReplaceOther(const boost::shared_ptr<ChimeraTK::TransferElement const>& other) const override {
      auto casted = boost::dynamic_pointer_cast<TypeChangingDecorator<T, IMPL_T> const>(other);
      if(!casted) return false;
      return _target->mayReplaceOther(casted->_target);
    }

   protected:
    using ChimeraTK::NDRegisterAccessorDecorator<T, IMPL_T>::_target;
  };

  /** This class is intended as a base class.
   *  It provides only partial template specialisations for the string stuff.
   * Don't directly instantiate this class. The factory would fail when looping
   * over all implementation types.
   */
  template<class T, class IMPL_T>
  class TypeChangingStringImplDecorator : public TypeChangingDecorator<T, IMPL_T> {
   public:
    using TypeChangingDecorator<T, IMPL_T>::TypeChangingDecorator;
  };

  /** This class is intended as a base class.
   *  It provides only partial template specialisations for the Void stuff.
   * Don't directly instantiate this class. The factory would fail when looping
   * over all implementation types.
   */
  template<class T, class IMPL_T>
  class TypeChangingVoidImplDecorator : public TypeChangingDecorator<T, IMPL_T> {
   public:
    using TypeChangingDecorator<T, IMPL_T>::TypeChangingDecorator;
  };

  /// A sub-namespace in order not to expose the classes to the ChimeraTK
  /// namespace
  namespace csa_helpers {

    template<class T>
    T stringToT(std::string const& input) {
      std::stringstream s;
      s << input;
      T t;
      s >> t;
      return t;
    }

    /// special treatment for int8_t, which is otherwise treated as a character/letter
    template<>
    int8_t stringToT<int8_t>(std::string const& input);

    /// special treatment for uint8_t, which is otherwise treated as a character/letter
    template<>
    uint8_t stringToT<uint8_t>(std::string const& input);

    template<class T>
    std::string T_ToString(T input) {
      std::stringstream s;
      s << input;
      std::string output;
      s >> output;
      return output;
    }

    /// special treatment for uint8_t, which is otherwise treated as a character/letter
    template<>
    std::string T_ToString<uint8_t>(uint8_t input);

    /// special treatment for int8_t, which is otherwise treated as a character/letter
    template<>
    std::string T_ToString<int8_t>(int8_t input);

    template<class S>
    struct Round {
      static S nearbyint(S s) { return round(s); }

      typedef boost::mpl::integral_c<std::float_round_style, std::round_to_nearest> round_style;
    };
  } // namespace csa_helpers

  /** The actual partial implementation for strings as impl type **/
  template<class T>
  class TypeChangingStringImplDecorator<T, std::string> : public TypeChangingDecorator<T, std::string> {
   public:
    using TypeChangingDecorator<T, std::string>::TypeChangingDecorator;
    void convertAndCopyFromImpl() override {
      for(size_t i = 0; i < this->buffer_2D.size(); ++i) {
        for(size_t j = 0; j < this->buffer_2D[i].size(); ++j) {
          this->buffer_2D[i][j] = csa_helpers::stringToT<T>(this->_target->accessChannel(i)[j]);
        }
      }
    }
    void convertAndCopyToImpl() override {
      for(size_t i = 0; i < this->buffer_2D.size(); ++i) {
        for(size_t j = 0; j < this->buffer_2D[i].size(); ++j) {
          this->_target->accessChannel(i)[j] = csa_helpers::T_ToString(this->buffer_2D[i][j]);
        }
      }
    }
  };

  /** The actual partial implementation for strings as user type */
  template<class IMPL_T>
  class TypeChangingStringImplDecorator<std::string, IMPL_T> : public TypeChangingDecorator<std::string, IMPL_T> {
   public:
    using TypeChangingDecorator<std::string, IMPL_T>::TypeChangingDecorator;
    void convertAndCopyFromImpl() override {
      for(size_t i = 0; i < this->buffer_2D.size(); ++i) {
        for(size_t j = 0; j < this->buffer_2D[i].size(); ++j) {
          this->buffer_2D[i][j] = csa_helpers::T_ToString(this->_target->accessChannel(i)[j]);
        }
      }
    }
    void convertAndCopyToImpl() override {
      for(size_t i = 0; i < this->buffer_2D.size(); ++i) {
        for(size_t j = 0; j < this->buffer_2D[i].size(); ++j) {
          this->_target->accessChannel(i)[j] = csa_helpers::stringToT<IMPL_T>(this->buffer_2D[i][j]);
        }
      }
    }
  };

  /** The actual partial implementation for Void as impl type **/
  template<class T>
  class TypeChangingStringImplDecorator<T, ChimeraTK::Void> : public TypeChangingDecorator<T, ChimeraTK::Void> {
   public:
    using TypeChangingDecorator<T, ChimeraTK::Void>::TypeChangingDecorator;
    void convertAndCopyFromImpl() override {
      for(size_t i = 0; i < this->buffer_2D.size(); ++i) {
        for(size_t j = 0; j < this->buffer_2D[i].size(); ++j) {
          this->buffer_2D[i][j] = {};
        }
      }
    }
    void convertAndCopyToImpl() override {}
  };

  /** The actual partial implementation for Void as user type */
  template<class IMPL_T>
  class TypeChangingStringImplDecorator<ChimeraTK::Void, IMPL_T>
  : public TypeChangingDecorator<ChimeraTK::Void, IMPL_T> {
   public:
    using TypeChangingDecorator<ChimeraTK::Void, IMPL_T>::TypeChangingDecorator;
    void convertAndCopyFromImpl() override {}
    void convertAndCopyToImpl() override {
      for(size_t i = 0; i < this->buffer_2D.size(); ++i) {
        for(size_t j = 0; j < this->buffer_2D[i].size(); ++j) {
          this->_target->accessChannel(i)[j] = {};
        }
      }
    }
  };

  /** The actual partial implementation for std::string as user type and Void as impl type **/
  template<>
  class TypeChangingStringImplDecorator<std::string, ChimeraTK::Void>
  : public TypeChangingDecorator<std::string, ChimeraTK::Void> {
   public:
    using TypeChangingDecorator<std::string, ChimeraTK::Void>::TypeChangingDecorator;
    void convertAndCopyFromImpl() override {
      for(size_t i = 0; i < this->buffer_2D.size(); ++i) {
        for(size_t j = 0; j < this->buffer_2D[i].size(); ++j) {
          this->buffer_2D[i][j] = {};
        }
      }
    }
    void convertAndCopyToImpl() override {}
  };

  /** The actual partial implementation for Void as user type and std::string as impl type */
  template<>
  class TypeChangingStringImplDecorator<ChimeraTK::Void, std::string>
  : public TypeChangingDecorator<ChimeraTK::Void, std::string> {
   public:
    using TypeChangingDecorator<ChimeraTK::Void, std::string>::TypeChangingDecorator;
    void convertAndCopyFromImpl() override {}
    void convertAndCopyToImpl() override {
      for(size_t i = 0; i < this->buffer_2D.size(); ++i) {
        for(size_t j = 0; j < this->buffer_2D[i].size(); ++j) {
          this->_target->accessChannel(i)[j] = {};
        }
      }
    }
  };

  /** The actual partial implementation for Void as user type and Void as impl type */
  template<>
  class TypeChangingStringImplDecorator<ChimeraTK::Void, ChimeraTK::Void>
  : public TypeChangingDecorator<ChimeraTK::Void, ChimeraTK::Void> {
   public:
    using TypeChangingDecorator<ChimeraTK::Void, ChimeraTK::Void>::TypeChangingDecorator;
    void convertAndCopyFromImpl() override {}
    void convertAndCopyToImpl() override {}
  };

  /** This decorator uses the boost numeric converter which performs two tasks:
   *
   *  - a range check (throws boost::numeric::bad_numeric_cast if out of range)
   *  - the rounding floating point -> int is done mathematically (not just cut
   * off the bits)
   */
  template<class T, class IMPL_T>
  class TypeChangingRangeCheckingDecorator : public TypeChangingStringImplDecorator<T, IMPL_T> {
   public:
    using TypeChangingStringImplDecorator<T, IMPL_T>::TypeChangingStringImplDecorator;
    void convertAndCopyFromImpl() override;
    void convertAndCopyToImpl() override;
    DecoratorType getDecoratorType() const override { return DecoratorType::limiting; }

   private:
    /** Internal exceptions to overload the what() function of the boost
     * exceptions in order to fill in the variable name. These exceptions are not
     * part of the external interface and cannot be caught explicitly because they
     * are protected. Catch boost::numeric::bad_numeric_cast and derrivatives if
     * you want to do error handling.
     */
    template<class BOOST_EXCEPTION_T>
    struct BadNumericCastException : public BOOST_EXCEPTION_T {
      BadNumericCastException(std::string variableName)
      : errorMessage(
            "Exception during type changing conversion in " + variableName + ": " + BOOST_EXCEPTION_T().what()) {}
      std::string errorMessage;
      const char* what() const throw() override { return errorMessage.c_str(); }
    };

    using ChimeraTK::NDRegisterAccessorDecorator<T, IMPL_T>::_target;
  };

  template<class T>
  class TypeChangingRangeCheckingDecorator<T, std::string> : public TypeChangingStringImplDecorator<T, std::string> {
   public:
    using TypeChangingStringImplDecorator<T, std::string>::TypeChangingStringImplDecorator;
    // Do not override convertAndCopyFromImpl() and convertAndCopyToImpl().
    // Use the implementations from TypeChangingStringImplDecorator<T, std::string>

    DecoratorType getDecoratorType() const override { return DecoratorType::limiting; }

   protected:
    //    using ChimeraTK::NDRegisterAccessorDecorator<T, std::string>::_target;
  };

  template<class IMPL_T>
  class TypeChangingRangeCheckingDecorator<std::string, IMPL_T>
  : public TypeChangingStringImplDecorator<std::string, IMPL_T> {
   public:
    using TypeChangingStringImplDecorator<std::string, IMPL_T>::TypeChangingStringImplDecorator;
    // Do not override convertAndCopyFromImpl() and convertAndCopyToImpl().
    // Use the implementations from TypeChangingStringImplDecorator<std::string, IMPL_T>

    DecoratorType getDecoratorType() const override { return DecoratorType::limiting; }

   protected:
  };

  template<class T>
  class TypeChangingRangeCheckingDecorator<T, ChimeraTK::Void>
  : public TypeChangingStringImplDecorator<T, ChimeraTK::Void> {
   public:
    using TypeChangingStringImplDecorator<T, ChimeraTK::Void>::TypeChangingStringImplDecorator;
    // Do not override convertAndCopyFromImpl() and convertAndCopyToImpl().
    // Use the implementations from TypeChangingStringImplDecorator<T, ChimeraTK::Void>

    DecoratorType getDecoratorType() const override { return DecoratorType::limiting; }

   protected:
    //    using ChimeraTK::NDRegisterAccessorDecorator<T, ChimeraTK::Void>::_target;
  };

  template<class IMPL_T>
  class TypeChangingRangeCheckingDecorator<ChimeraTK::Void, IMPL_T>
  : public TypeChangingStringImplDecorator<ChimeraTK::Void, IMPL_T> {
   public:
    using TypeChangingStringImplDecorator<ChimeraTK::Void, IMPL_T>::TypeChangingStringImplDecorator;
    // Do not override convertAndCopyFromImpl() and convertAndCopyToImpl().
    // Use the implementations from TypeChangingStringImplDecorator<ChimeraTK::Void, IMPL_T>

    DecoratorType getDecoratorType() const override { return DecoratorType::limiting; }

   protected:
  };

  template<>
  class TypeChangingRangeCheckingDecorator<std::string, ChimeraTK::Void>
  : public TypeChangingStringImplDecorator<std::string, ChimeraTK::Void> {
   public:
    using TypeChangingStringImplDecorator<std::string, ChimeraTK::Void>::TypeChangingStringImplDecorator;
    // Do not override convertAndCopyFromImpl() and convertAndCopyToImpl().
    // Use the implementations from TypeChangingStringImplDecorator<std::string, ChimeraTK::Void>

    DecoratorType getDecoratorType() const override { return DecoratorType::limiting; }

   protected:
    //    using ChimeraTK::NDRegisterAccessorDecorator<T, ChimeraTK::Void>::_target;
  };

  template<>
  class TypeChangingRangeCheckingDecorator<ChimeraTK::Void, std::string>
  : public TypeChangingStringImplDecorator<ChimeraTK::Void, std::string> {
   public:
    using TypeChangingStringImplDecorator<ChimeraTK::Void, std::string>::TypeChangingStringImplDecorator;
    // Do not override convertAndCopyFromImpl() and convertAndCopyToImpl().
    // Use the implementations from TypeChangingStringImplDecorator<ChimeraTK::Void, std::string>

    DecoratorType getDecoratorType() const override { return DecoratorType::limiting; }

   protected:
  };

  template<>
  class TypeChangingRangeCheckingDecorator<ChimeraTK::Void, ChimeraTK::Void>
  : public TypeChangingStringImplDecorator<ChimeraTK::Void, ChimeraTK::Void> {
   public:
    using TypeChangingStringImplDecorator<ChimeraTK::Void, ChimeraTK::Void>::TypeChangingStringImplDecorator;
    // Do not override convertAndCopyFromImpl() and convertAndCopyToImpl().
    // Use the implementations from TypeChangingStringImplDecorator<ChimeraTK::Void, std::string>

    DecoratorType getDecoratorType() const override { return DecoratorType::limiting; }

   protected:
  };

  /********************************************************************************************************************/
  /*** Implementations of member functions below this line ************************************************************/
  /********************************************************************************************************************/

  template<class T, class IMPL_T>
  TypeChangingDecorator<T, IMPL_T>::TypeChangingDecorator(
      boost::shared_ptr<ChimeraTK::NDRegisterAccessor<IMPL_T>> const& target) noexcept
  : ChimeraTK::NDRegisterAccessorDecorator<T, IMPL_T>(target) {}

  /********************************************************************************************************************/

  template<class T, class IMPL_T>
  void TypeChangingRangeCheckingDecorator<T, IMPL_T>::convertAndCopyFromImpl() {
    typedef boost::numeric::converter<T, IMPL_T, boost::numeric::conversion_traits<T, IMPL_T>,
        boost::numeric::def_overflow_handler, csa_helpers::Round<double>>
        FromImplConverter;
    // fixme: are iterartors more efficient?
    for(size_t i = 0; i < this->buffer_2D.size(); ++i) {
      for(size_t j = 0; j < this->buffer_2D[i].size(); ++j) {
        try {
          this->buffer_2D[i][j] = FromImplConverter::convert(_target->accessChannel(i)[j]);
        }
        catch(boost::numeric::positive_overflow&) {
          this->buffer_2D[i][j] = std::numeric_limits<T>::max();
        }
        catch(boost::numeric::negative_overflow&) {
          this->buffer_2D[i][j] = std::numeric_limits<T>::min();
        }
      }
    }
  }

  template<class T, class IMPL_T>
  void TypeChangingRangeCheckingDecorator<T, IMPL_T>::convertAndCopyToImpl() {
    typedef boost::numeric::converter<IMPL_T, T, boost::numeric::conversion_traits<IMPL_T, T>,
        boost::numeric::def_overflow_handler, csa_helpers::Round<double>>
        ToImplConverter;
    for(size_t i = 0; i < this->buffer_2D.size(); ++i) {
      for(size_t j = 0; j < this->buffer_2D[i].size(); ++j) {
        try {
          _target->accessChannel(i)[j] = ToImplConverter::convert(this->buffer_2D[i][j]);
        }
        catch(boost::numeric::positive_overflow&) {
          _target->accessChannel(i)[j] = std::numeric_limits<IMPL_T>::max();
        }
        catch(boost::numeric::negative_overflow&) {
          _target->accessChannel(i)[j] = std::numeric_limits<IMPL_T>::min();
        }
      }
    }
  }

  /** This decorator does not do mathematical rounding and range checking, but
   *  directly assigns the data types (C-style direct conversion).
   *  Probably the only useful screnario is the conversion int/uint if negative
   * data should be displayed as the last half of the dynamic range, or vice
   * versa. (e.g. 0xFFFFFFFF <-> -1).
   */
  template<class T, class IMPL_T>
  class TypeChangingDirectCastDecorator : public TypeChangingStringImplDecorator<T, IMPL_T> {
   public:
    using TypeChangingStringImplDecorator<T, IMPL_T>::TypeChangingStringImplDecorator;
    DecoratorType getDecoratorType() const override { return DecoratorType::C_style_conversion; }

    void convertAndCopyFromImpl() override {
      // fixme: are iterartors more efficient?
      for(size_t i = 0; i < this->buffer_2D.size(); ++i) {
        for(size_t j = 0; j < this->buffer_2D[i].size(); ++j) {
          this->buffer_2D[i][j] = _target->accessChannel(i)[j];
        }
      }
    }

    void convertAndCopyToImpl() override {
      for(size_t i = 0; i < this->buffer_2D.size(); ++i) {
        for(size_t j = 0; j < this->buffer_2D[i].size(); ++j) {
          _target->accessChannel(i)[j] = this->buffer_2D[i][j];
        }
      }
    }

    using ChimeraTK::NDRegisterAccessorDecorator<T, IMPL_T>::_target;
  };

  /** Partial template specialisation for strings as impl type.
   */
  template<class T>
  class TypeChangingDirectCastDecorator<T, std::string> : public TypeChangingStringImplDecorator<T, std::string> {
   public:
    using TypeChangingStringImplDecorator<T, std::string>::TypeChangingStringImplDecorator;
    DecoratorType getDecoratorType() const override { return DecoratorType::C_style_conversion; }
  };

  /** Partial template specialisation for strings as user type.
   */
  template<class IMPL_T>
  class TypeChangingDirectCastDecorator<std::string, IMPL_T>
  : public TypeChangingStringImplDecorator<std::string, IMPL_T> {
   public:
    using TypeChangingStringImplDecorator<std::string, IMPL_T>::TypeChangingStringImplDecorator;
    DecoratorType getDecoratorType() const override { return DecoratorType::C_style_conversion; }
  };

  /** Partial template specialisation for Void as impl type.
   */
  template<class T>
  class TypeChangingDirectCastDecorator<T, ChimeraTK::Void>
  : public TypeChangingStringImplDecorator<T, ChimeraTK::Void> {
   public:
    using TypeChangingStringImplDecorator<T, ChimeraTK::Void>::TypeChangingStringImplDecorator;
    DecoratorType getDecoratorType() const override { return DecoratorType::C_style_conversion; }
  };

  /** Partial template specialisation for Void as user type.
   */
  template<class IMPL_T>
  class TypeChangingDirectCastDecorator<ChimeraTK::Void, IMPL_T>
  : public TypeChangingStringImplDecorator<ChimeraTK::Void, IMPL_T> {
   public:
    using TypeChangingStringImplDecorator<ChimeraTK::Void, IMPL_T>::TypeChangingStringImplDecorator;
    DecoratorType getDecoratorType() const override { return DecoratorType::C_style_conversion; }
  };

  /** template specialisation for Void as impl type and string as user type.
   */
  template<>
  class TypeChangingDirectCastDecorator<std::string, ChimeraTK::Void>
  : public TypeChangingStringImplDecorator<std::string, ChimeraTK::Void> {
   public:
    using TypeChangingStringImplDecorator<std::string, ChimeraTK::Void>::TypeChangingStringImplDecorator;
    DecoratorType getDecoratorType() const override { return DecoratorType::C_style_conversion; }
  };

  /** template specialisation for Void as user type and string as impl type.
   */
  template<>
  class TypeChangingDirectCastDecorator<ChimeraTK::Void, std::string>
  : public TypeChangingStringImplDecorator<ChimeraTK::Void, std::string> {
   public:
    using TypeChangingStringImplDecorator<ChimeraTK::Void, std::string>::TypeChangingStringImplDecorator;
    DecoratorType getDecoratorType() const override { return DecoratorType::C_style_conversion; }
  };

  /** template specialisation for Void as user type and string as impl type.
   */
  template<>
  class TypeChangingDirectCastDecorator<ChimeraTK::Void, ChimeraTK::Void>
  : public TypeChangingStringImplDecorator<ChimeraTK::Void, ChimeraTK::Void> {
   public:
    using TypeChangingStringImplDecorator<ChimeraTK::Void, ChimeraTK::Void>::TypeChangingStringImplDecorator;
    DecoratorType getDecoratorType() const override { return DecoratorType::C_style_conversion; }
  };

  /** Key type for the global decorator map. */
  struct DecoratorMapKey {
    boost::shared_ptr<ChimeraTK::TransferElement> element;
    const std::type_info& dataType;
    ChimeraTK::DecoratorType conversionType;
    bool operator<(const DecoratorMapKey& other) const {
      if(element < other.element) return true;
      if(element != other.element) return false;
      if(&dataType < &other.dataType) return true;
      if(&dataType != &other.dataType) return false;
      if(conversionType < other.conversionType) return true;
      return false;
    }
    bool operator==(const DecoratorMapKey& other) const {
      return element == other.element && dataType == other.dataType && conversionType == other.conversionType;
    }
  };

  /** Quasi singleton to have a unique, global map across UserType templated
   * factories. We need it to loop up if a decorator has already been created for
   * the transfer element, and return this if so. Multiple decorators for the same
   * transfer element don't work.
   */
  std::map<DecoratorMapKey, boost::shared_ptr<ChimeraTK::TransferElement>>& getGlobalDecoratorMap();

  template<class UserType>
  class DecoratorFactory {
   public:
    DecoratorFactory(boost::shared_ptr<ChimeraTK::TransferElement> theImpl_, DecoratorType wantedDecoratorType_)
    : theImpl(theImpl_), wantedDecoratorType(wantedDecoratorType_) {}
    boost::shared_ptr<ChimeraTK::TransferElement> theImpl;
    DecoratorType wantedDecoratorType;
    mutable boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> createdDecorator;

    DecoratorFactory(const DecoratorFactory&) = delete;

    template<typename PAIR>
    void operator()(PAIR&) const {
      typedef typename PAIR::first_type TargetImplType;
      if(typeid(TargetImplType) != theImpl->getValueType()) return;

      if(wantedDecoratorType == DecoratorType::limiting) {
        createdDecorator.reset(new TypeChangingRangeCheckingDecorator<UserType, TargetImplType>(
            boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<TargetImplType>>(theImpl)));
      }
      else if(wantedDecoratorType == DecoratorType::C_style_conversion) {
        createdDecorator.reset(new TypeChangingDirectCastDecorator<UserType, TargetImplType>(
            boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<TargetImplType>>(theImpl)));
      }
      else {
        throw ChimeraTK::logic_error("TypeChangingDecorator with range "
                                     "limitation is not implemented yet.");
      }
    }
  };

  // the factory function. You don't have to care about the IMPL_Type when
  // requesting a decorator. Implementation, declared at the top of this file
  template<class UserType>
  boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> getTypeChangingDecorator(
      const boost::shared_ptr<ChimeraTK::TransferElement>& transferElement, DecoratorType decoratorType) {
    // check if there already is a decorator for the transfer element with the right type
    DecoratorMapKey key{transferElement, typeid(UserType), decoratorType};
    auto decoratorMapEntry = getGlobalDecoratorMap().find(key);
    if(decoratorMapEntry != getGlobalDecoratorMap().end()) {
      // There already is a decorator for this transfer element
      // The decorator has to have a matching type, otherwise we can only throw
      auto castedType = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<UserType>>(decoratorMapEntry->second);
      assert(castedType);
      assert(boost::dynamic_pointer_cast<DecoratorTypeHolder>(decoratorMapEntry->second));
      return castedType;
    }

    DecoratorFactory<UserType> factory(transferElement, decoratorType);
    boost::fusion::for_each(ChimeraTK::userTypeMap(), std::ref(factory));
    if(factory.createdDecorator == nullptr) {
      throw ChimeraTK::logic_error("ChimeraTK::ControlSystemAdapter: Decorator for TransferElement " +
          transferElement->getName() + " has been requested for an unknown user type: " + typeid(UserType).name());
    }
    getGlobalDecoratorMap()[key] = factory.createdDecorator;
    return factory.createdDecorator;
  }

} // namespace ChimeraTK
