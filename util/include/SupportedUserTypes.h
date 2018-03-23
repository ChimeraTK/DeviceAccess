/*
 * SupportedUserTypes.h - Define boost::fusion::maps of the user data types supported by the CHIMERA_TK library.
 *
 *  Created on: Feb 29, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_SUPPORTED_USER_TYPES_H
#define CHIMERA_TK_SUPPORTED_USER_TYPES_H

#include <boost/fusion/container/map.hpp>

namespace ChimeraTK {

  /** Map of UserType to value of the UserType. Used e.g. by the FixedPointConverter to store coefficients etc. in
   *  dependence of the UserType. */
  typedef boost::fusion::map<
      boost::fusion::pair<int8_t,int8_t>,
      boost::fusion::pair<uint8_t,uint8_t>,
      boost::fusion::pair<int16_t,int16_t>,
      boost::fusion::pair<uint16_t,uint16_t>,
      boost::fusion::pair<int32_t,int32_t>,
      boost::fusion::pair<uint32_t,uint32_t>,
      boost::fusion::pair<int64_t,int64_t>,
      boost::fusion::pair<uint64_t,uint64_t>,
      boost::fusion::pair<float,float>,
      boost::fusion::pair<double,double>,
      boost::fusion::pair<std::string,std::string>
  > userTypeMap;

  /** Map of UserType to a class template with the UserType as template argument. Used e.g. by the
   *  VirtualFunctionTemplate macros to implement the vtable. */
  template< template<typename> class TemplateClass >
  class TemplateUserTypeMap {
    public:
      boost::fusion::map<
          boost::fusion::pair<int8_t, TemplateClass<int8_t> >,
          boost::fusion::pair<uint8_t, TemplateClass<uint8_t> >,
          boost::fusion::pair<int16_t, TemplateClass<int16_t> >,
          boost::fusion::pair<uint16_t, TemplateClass<uint16_t> >,
          boost::fusion::pair<int32_t, TemplateClass<int32_t> >,
          boost::fusion::pair<uint32_t, TemplateClass<uint32_t> >,
          boost::fusion::pair<int64_t, TemplateClass<int64_t> >,
          boost::fusion::pair<uint64_t, TemplateClass<uint64_t> >,
          boost::fusion::pair<float, TemplateClass<float> >,
          boost::fusion::pair<double, TemplateClass<double> >,
          boost::fusion::pair<std::string, TemplateClass<std::string> >
       > table;
  };

  /** Map of UserType to a single type */
  template<typename T>
  using SingleTypeUserTypeMap = boost::fusion::map<
                                    boost::fusion::pair<int8_t,T>,
                                    boost::fusion::pair<uint8_t,T>,
                                    boost::fusion::pair<int16_t,T>,
                                    boost::fusion::pair<uint16_t,T>,
                                    boost::fusion::pair<int32_t,T>,
                                    boost::fusion::pair<uint32_t,T>,
                                    boost::fusion::pair<int64_t,T>,
                                    boost::fusion::pair<uint64_t,T>,
                                    boost::fusion::pair<float,T>,
                                    boost::fusion::pair<double,T>,
                                    boost::fusion::pair<std::string,T>
                                > ;

#define DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES( TemplateClass ) \
  extern template class TemplateClass<int8_t>;  \
  extern template class TemplateClass<uint8_t>;  \
  extern template class TemplateClass<int16_t>; \
  extern template class TemplateClass<uint16_t>; \
  extern template class TemplateClass<int32_t>; \
  extern template class TemplateClass<uint32_t>; \
  extern template class TemplateClass<int64_t>; \
  extern template class TemplateClass<uint64_t>; \
  extern template class TemplateClass<float>;   \
  extern template class TemplateClass<double>;  \
  extern template class TemplateClass<std::string>// the last semicolon is added by the user

#define INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES( TemplateClass )      \
  template class TemplateClass<int8_t>;  \
  template class TemplateClass<uint8_t>;  \
  template class TemplateClass<int16_t>; \
  template class TemplateClass<uint16_t>; \
  template class TemplateClass<int32_t>; \
  template class TemplateClass<uint32_t>; \
  template class TemplateClass<int64_t>; \
  template class TemplateClass<uint64_t>; \
  template class TemplateClass<float>;   \
  template class TemplateClass<double>;  \
  template class TemplateClass<std::string>// the last semicolon is added by the user

  /** A class to describe which of the supported data types is used.
   *  There is the additional type 'none' to indicate that the data type is not available in
   *  the current context. For instance if DataType is used to identify the raw data type
   *  of an accessor, the value is none if the accessor does not have a raw transfer mechanism.
   *
   *  The DataType behaves almost like a class enum due to the plain enum inside it
   *  (see description of the <tt>operator TheType & ()</tt> ).
   */
  class DataType{
    public:
      /** The actual enum representing the data type. It is a plain enum so 
       *  the data type class can be used like a class enum, i.e. types are 
       *  identified for instance as DataType::int32.
       */
      enum TheType{ none, ///< The data type/concept does not exist. e.g. there is no raw transfer 
                    int8, ///< Signed 8 bit integer
                    uint8, ///< Unsigned 8 bit integer
                    int16,///< Signed 16 bit integer
                    uint16,///< Unsigned 16 bit integer
                    int32,///< Signed 32 bit integer
                    uint32,///< Unsigned 32 bit integer
                    int64,///< Signed 64 bit integer
                    uint64,///< Unsigned 64 bit integer
                    float32,///< Single precision float
                    float64,///< Double precision float
                    string ///< std::string
      };

      /** Implicit conversion operator to make DataType behave like a class enum.
       *  Allows for instance comparison or assigment with members of the TheType enum.
       *  \code
       DataType myDataType; // default constructor. The type now is 'none'.

       // DataType::int32 is of type DataType::TheType. The implicit conversion of \c myDataType
       // allows the assignment.
       myDataType = DataType::int32;
       \endcode
       */
      inline operator TheType & (){
        return _value;
      }

      /** Return whether the raw data type is an integer.
       *  False is also returned for non-numerical types and 'none'.
       */
      inline bool isIntegral() const{
        switch (_value){
          case int8:
          case uint8:
          case int16:
          case uint16:
          case int32:
          case uint32:
          case int64:
          case uint64:
            return true;
          default:
            return false;
        }
      }

      /** Return whether the raw data type is signed. True for signed integers and 
       *  floating point types (currently only signed implementations).
       *  False otherwise (also for non-numerical types and 'none').
       */
      inline bool isSigned() const{
        switch (_value){
          case int8:
          case int16:
          case int32:
          case int64:
          case float32:
          case float64:
            return true;
          default:
            return false;
        }
      }

      /** Returns whether the data type is numeric.
       *  Type 'none' returns false.
       */
      inline bool isNumeric() const{
        // I inverted the logic to minimise the amout of code. If you add non-numeric types
        // this has to be adapted.
        switch (_value){
          case none:
          case string:
            return false;
          default:
            return true;
        }
      }
      
      /** The constructor can get the type as an argument. It defaults to 'none'.
       */
      inline DataType( TheType const & value = none ): _value(value){}
      
    protected:
      TheType _value;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERA_TK_SUPPORTED_USER_TYPES_H */
