// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <boost/bind/bind.hpp>
#include <boost/function.hpp>
#include <boost/fusion/include/at_key.hpp>
#include <boost/fusion/include/for_each.hpp>

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
 * filled differently depending on whether you want to (be able to) call the base class's
 * implementation or not:
 *
 * <ul> If you do not care about the base implementation, use the macros
 * DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER
 * and FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE.
 * <ul> If you want to be able to delegate to the base implementation, use
 * the macros DEFINE_VIRTUAL_FUNCTION_OVERRIDE_VTABLE and OVERRIDE_VIRTUAL_FUNCTION_TEMPLATE. To chain up to the base
 * implementation, use the macro CALL_BASE_FUNCTION_TEMPLATE
 *
 *  Note: the signature is passed through the __VA_ARGS__ variable macro
 * arguments, as it may contain commas.
 */
#define DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(functionName, ...)                                                     \
  template<typename T>                                                                                                 \
  class functionName##_functionSignature : public boost::function<__VA_ARGS__> {                                       \
   public:                                                                                                             \
    functionName##_functionSignature& operator=(const boost::function<__VA_ARGS__>& rhs) {                             \
      boost::function<__VA_ARGS__>::operator=(rhs);                                                                    \
      return *this;                                                                                                    \
    }                                                                                                                  \
  };                                                                                                                   \
  TemplateUserTypeMap<functionName##_functionSignature> functionName##_vtable

/** Define an override for a function template with the given function name and
 * signature in the base class.
 *
 * Semantics apply as in DEFINE_VIRTUAL_FUNCTION_TEMPLATE.
 */
#define DEFINE_VIRTUAL_FUNCTION_OVERRIDE_VTABLE(BaseClass, functionName, ...)                                          \
  DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(BaseClass##functionName, __VA_ARGS__)

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

/** Execute the virtual function template call to the base implementation of the function.
 *
 *  The function must have been overriden by DEFINE_VIRTUAL_FUNCTION_OVERRIDE_VTABLE and
 *  OVERRIDE_VIRTUAL_FUNCTION_TEMPLATE
 */
#define CALL_BASE_FUNCTION_TEMPLATE(BaseClass, functionName, templateArgument, ...)                                    \
  boost::fusion::at_key<templateArgument>(BaseClass##functionName##_vtable.table)(__VA_ARGS__)

/** Fill the vtable of a virtual function template defined with
 * DEFINE_VIRTUAL_FUNCTION_TEMPLATE. Use this macro inside the constructor of
 * the derived class. */
#define FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(functionName)                                                            \
  ChimeraTK::for_each(this->functionName##_vtable.table, [this](auto& pair) {                                          \
    typedef typename std::remove_reference<decltype(pair)>::type::first_type VTableFillerUserType;                     \
    pair.second = [this](auto... args) { return this->functionName<VTableFillerUserType>(args...); };                  \
  })

/** Save the old vtable to be accessible by CALL_BASE_FUNCTION_TEMPLATE and overwrite it with the
 *  new implementation.
 */
#define OVERRIDE_VIRTUAL_FUNCTION_TEMPLATE(BaseClass, functionName)                                                    \
  do {                                                                                                                 \
    /* Copy the old vtable into _base before filling it with the new implementation */                                 \
    ChimeraTK::for_each(BaseClass##functionName##_vtable.table, [&](auto& pair) {                                      \
      typedef typename std::remove_reference<decltype(pair)>::type::first_type VTableFillerUserType;                   \
      pair.second = (boost::fusion::at_key<VTableFillerUserType>(functionName##_vtable.table));                        \
    });                                                                                                                \
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(functionName);                                                               \
  } while(false)

/** Compatibility only, do not use */
#define FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_STANDALONE(functionName, numberOfArguments)                              \
  FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(functionName)

/** Compatibility, do not use. */
#define DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(className, functionName, numberOfArguments)                     \
  class UslessVTableFillerClass##functionName {}
