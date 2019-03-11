/*
 * VirtualFunctionTemplate.h - Set of macros to approximate templated virtual
 * functions
 *
 *  Created on: Feb 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_VIRTUAL_FUNCTION_TEMPLATE_H
#define CHIMERA_TK_VIRTUAL_FUNCTION_TEMPLATE_H

/* We need special compiler flags for boost fusion.
 * Make sure they are set before the functions are included.
 *
 * Note: We cannot just set them here because the boost fusion headers might
 * have been included before, without the flags, so the follwing includes have
 * no effect and the files are already included with the wrong parameters.
 */
#if FUSION_MAX_MAP_SIZE != 30 || FUSION_MAX_VECTOR_SIZE != 30
#  error The sizes for boost::fusion are not set correctly as compiler flags.
#  error Include the compiler flags provided by chimeratk-deviceaccess:
#  error* In cmake: set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ChimeraTK-DeviceAccess_CXX_FLAGS}")
#  error* in standard Makefiles: CPPFLAGS += $(shell ChimeraTK-DeviceAccess-config --cppflags)
#endif

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/fusion/include/at_key.hpp>
#include <boost/fusion/include/for_each.hpp>

#include <tuple>

#include "SupportedUserTypes.h"

/** Define a virtual function template with the given function name and
 * signature in the base class. The signature must contain the typename template
 * argument called "T". So if you e.g. would want to define a function like
 * this:
 *
 *    template<typename UserType>
 *    virtual BufferingRegisterAccessor<UserType>
 * getRegisterAccessor(std::string name);
 *
 *  you should call this macro in the base class like this:
 *
 *    DEFINE_VIRTUAL_FUNCTION_TEMPLATE( getRegisterAccessor, T(std::string) );
 *
 *  The virtual function can be called using the CALL_VIRTUAL_FUNCTION_TEMPLATE
 * macro. It is recommended to define the function template in the base class
 * (without the virtual keyword of course) and implement it by using this macro.
 *
 *  In the derived class, the function template must be implemented with the
 * same signature, and the vtable for this virtual function template must be
 * filled using the macros DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER and
 *  FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE.
 *
 *  Note: the signature is passed through the __VA_ARGS__ variable macro
 * arguments, as it may contain commas.
 */
#define DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(functionName, ...)                                                     \
  template<typename T>                                                                                                 \
  class functionName##_functionSignature : public boost::function<__VA_ARGS__> {                                       \
   public:                                                                                                             \
    boost::function<__VA_ARGS__>& operator=(const boost::function<__VA_ARGS__>& rhs) {                                 \
      return boost::function<__VA_ARGS__>::operator=(rhs);                                                             \
    }                                                                                                                  \
  };                                                                                                                   \
  TemplateUserTypeMap<functionName##_functionSignature> functionName##_vtable

/** Execute the virtual function template call using the vtable defined with the
 *  DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE macro. It is recommended to put this
 * macro into a function template with the same name and signature, so the user
 * of the class can call the virtual function template conveniently.
 *
 *  Implementation note: This function template could automatically be provided
 * via the DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE macro. This is not done,
 * because it wouldn't allow to specify default values for the parameters, and
 * the proper signature would not be visible in Doxygen. */
#define CALL_VIRTUAL_FUNCTION_TEMPLATE(functionName, templateArgument, ...)                                            \
  boost::fusion::at_key<templateArgument>(functionName##_vtable.table)(__VA_ARGS__)

/** Fill the vtable of a virtual function template defined with
 * DEFINE_VIRTUAL_FUNCTION_TEMPLATE. Use this macro inside the constructor of
 * the derived class. */
#define FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(functionName)                                                            \
  for_each(this->functionName##_vtable.table, [this](auto& pair) {                                                     \
    typedef typename std::remove_reference<decltype(pair)>::type::first_type VTableFillerUserType;                     \
    pair.second = [this](auto... args) { return this->functionName<VTableFillerUserType>(args...); };                  \
  })

/** Compatibility only, do not use */
#define FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_STANDALONE(functionName, numberOfArguments)                              \
  FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(functionName)

/** Compatibility, do not use. */
#define DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(className, functionName, numberOfArguments)                     \
  class UslessVTableFillerClass##functionName {}

#endif /* CHIMERA_TK_VIRTUAL_FUNCTION_TEMPLATE_H */
