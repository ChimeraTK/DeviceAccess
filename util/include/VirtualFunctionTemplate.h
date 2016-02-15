/*
 * VirtualFunctionTemplate.h - Set of macros to approximate templated virtual functions
 * Important: These macros are for use inside DeviceBackends only!
 *
 *  Created on: Feb 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_VIRTUAL_FUNCTION_TEMPLATE_H
#define MTCA4U_VIRTUAL_FUNCTION_TEMPLATE_H

#include <tuple>
#include "FixedPointConverter.h"

/** Macro for creating an approximation of a virtual template member function. It works only for a pre-defined
 *  list of types (FixedPointConverter::userTypeMap - i.e. all types supported by the FixedPointConverter) and
 *  will implement a runtime comparison of the given type_info with each type of the list.
 *
 *  The class name in which the virtual function should be implemented must be given as the first argument and
 *  must inherit from DeviceBackend. The second argument must be the name of the virtual function inside the class.
 *
 *  The third argument specifies the name of a non-member templated function to be called. This function must be
 *  declared before calling this macro and should contain the actual implementation. The return type of the
 *  function must be a plain pointer and its first argument must be a boost::shared_ptr<className> and will contain
 *  a pointer to the instance of the calling class.
 *
 *  A list of argument types to pass to the template function need to be passed as third and following arguments.
 *
 *  This macro shall be called inside the implementation .cc file of the class. It will implement the virtual
 *  function declared by the VIRTUAL_TEMPLATE_FUNCTION_DECLARATION macro in the class header file. The call to the
 *  macro should not be terminated with a semicolon.
 *
 *  Important: These macros are for use inside DeviceBackends only!
 */
#define VIRTUAL_FUNCTION_TEMPLATE_IMPLEMENTER( className, virtualFunction, templateFunction, ... )              \
    template<typename ...Args>                                                                                  \
    class className::virtualFunction ## _implClass {                                                            \
      public:                                                                                                   \
        virtualFunction ## _implClass(const std::type_info &_type, className *_self,                            \
                                 const std::tuple<Args...> &_args)                                              \
        : type(_type), args(_args), returnValue(NULL), success(false), self(_self)                              \
        {}                                                                                                      \
                                                                                                                \
        /* The type to call the template function for */                                                        \
        const std::type_info &type;                                                                             \
                                                                                                                \
        /* Arguments passed */                                                                                  \
        const std::tuple<Args...> &args;                                                                        \
                                                                                                                \
        /* pointer to the return value (if any) */                                                              \
        mutable void *returnValue;                                                                              \
                                                                                                                \
        /* flag if the function was executed (remains false, if unsupported type was passed) */                 \
        mutable bool success;                                                                                   \
                                                                                                                \
        /* instance of calling class */                                                                         \
        className *self;                                                                                        \
                                                                                                                \
        /* These definitions are needed to unpack the tuple into arguments.  */                                 \
        /* The int template argument is for the number of arguments. */                                         \
        template<int ...> struct seq {};                                                                        \
        template<int N, int ...S> struct gens : gens<N-1, N-1, S...> {};                                        \
        template<int ...S> struct gens<0, S...>{ typedef seq<S...> type; };                                     \
                                                                                                                \
        /* This function will be called by for_each() for each type in FixedPointConverter::userTypeMap */      \
        template <typename Pair>                                                                                \
        void operator()(Pair) const                                                                             \
        {                                                                                                       \
          /* obtain UserType from given fusion::pair type */                                                    \
          typedef typename Pair::first_type UserType;                                                           \
                                                                                                                \
          /* check if UserType is the requested type */                                                         \
          if(typeid(UserType) == type) {                                                                        \
            returnValue = callFunc<UserType>(typename gens<sizeof...(Args)>::type());                           \
            success = true;                                                                                     \
          }                                                                                                     \
        }                                                                                                       \
                                                                                                                \
        /* unpack arguments and call the function */                                                            \
        template<typename UserType, int ...S>                                                                   \
        void* callFunc(seq<S...>) const {                                                                       \
          return static_cast<void*>(self->templateFunction<UserType>(std::get<S>(args) ...));                   \
        }                                                                                                       \
    };                                                                                                          \
  void* className::virtualFunction(const std::type_info &UserType, std::tuple<__VA_ARGS__> args) {              \
    FixedPointConverter::userTypeMap userTypes;                                                                 \
    className::virtualFunction ## _implClass<__VA_ARGS__> myimpl(UserType, this, args);                        \
    boost::fusion::for_each(userTypes, myimpl);                                                                 \
    if(!myimpl.success) {                                                                                       \
      throw DeviceException("Unknown user type passed to " #className "::" #virtualFunction,                    \
          DeviceException::EX_WRONG_PARAMETER);                                                                 \
    }                                                                                                           \
    return myimpl.returnValue;                                                                                  \
  }

/********************************************************************************************************************/

/** Declare the virtual template function inside the class. It must be called inside the class definition in the
 *  header file. The call to this macro must be terminated with a semicolon. In the abstract base class the
 *  function can be marked as pure virtual by adding "= 0" before the semicolon. */
#define VIRTUAL_FUNCTION_TEMPLATE_DECLARATION(virtualFunction, ...)                                             \
    template<typename ...Args>                                                                                  \
    class virtualFunction ## _implClass;                                                                        \
    template<typename ...Args>                                                                                  \
    friend class virtualFunction ## _implClass;                                                                 \
    virtual void* virtualFunction(const std::type_info &UserType, std::tuple<__VA_ARGS__> args)

/********************************************************************************************************************/

/** Make the virtual template function call. Calls a virtual function named in the first argument and declared by
 * VIRTUAL_TEMPLATE_FUNCTION_DECLARATION and passes the template argument specified in the second argument. The
 * third argument specifies the return type, which must be a plain pointer type. The following arguments are the
 * parameters to pass to the function. This macro shall be called inside the implementation of a template function
 * in the base class. */
#define VIRTUAL_FUNCTION_TEMPLATE_CALL(virtualFunction, UserType, ReturnType, ...)                              \
    static_cast<ReturnType>( virtualFunction(typeid(UserType), std::make_tuple(__VA_ARGS__)) )


#endif /* MTCA4U_VIRTUAL_FUNCTION_TEMPLATE_H */
