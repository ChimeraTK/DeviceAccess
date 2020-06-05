/*
 * NDRegisterAccessorDecorator.cc
 *
 *  Created on: Dec 12 2017
 *      Author: Martin Hierholzer
 */

#include "NDRegisterAccessorDecorator.h"
#include "CopyRegisterDecorator.h"
#include "SupportedUserTypes.h"
#if 0
namespace ChimeraTK { namespace detail {

  template<typename T>
  boost::shared_ptr<ChimeraTK::NDRegisterAccessor<T>> createCopyDecorator(
      boost::shared_ptr<ChimeraTK::NDRegisterAccessor<T>> target) {
    return boost::make_shared<CopyRegisterDecorator<T>>(target);
  }

}} // namespace ChimeraTK::detail

namespace {

  // instantiate all needed implementations of the createCopyDecorator<T>()
  // function
  template<typename T>
  struct CreateCopyDecoratorInstancer {
    CreateCopyDecoratorInstancer<T>() : ptr(&ChimeraTK::detail::createCopyDecorator<T>) {}

    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<T>> (*ptr)(boost::shared_ptr<ChimeraTK::NDRegisterAccessor<T>>);
  };
  ChimeraTK::TemplateUserTypeMap<CreateCopyDecoratorInstancer> createCopyDecoratorInstancer;
} // namespace
#endif //0
