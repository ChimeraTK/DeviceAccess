/*
 * NDRegisterAccessorDecorator.cc
 *
 *  Created on: Dec 12 2017
 *      Author: Martin Hierholzer
 */

#include "NDRegisterAccessorDecorator.h"
#include "CopyRegisterDecorator.h"
#include "SupportedUserTypes.h"

namespace mtca4u {
  namespace detail {

    template<typename T>
    boost::shared_ptr<mtca4u::NDRegisterAccessor<T>> createCopyDecorator(boost::shared_ptr<mtca4u::NDRegisterAccessor<T>> target) {
      return boost::make_shared<CopyRegisterDecorator<T>>(target);
    }

  }
}

namespace {

  // instantiate all needed implementations of the createCopyDecorator<T>() function
  template<typename T>
  struct CreateCopyDecoratorInstancer {
      CreateCopyDecoratorInstancer<T>()
      : ptr(&mtca4u::detail::createCopyDecorator<T>)
      {}

      boost::shared_ptr<mtca4u::NDRegisterAccessor<T>> (*ptr)(boost::shared_ptr<mtca4u::NDRegisterAccessor<T>>);
  };
  mtca4u::TemplateUserTypeMap<CreateCopyDecoratorInstancer> createCopyDecoratorInstancer;

}
