// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "NDRegisterAccessorDecorator.h"

#include "CopyRegisterDecorator.h"
#include "SupportedUserTypes.h"

namespace ChimeraTK::detail {

  template<typename T>
  boost::shared_ptr<ChimeraTK::NDRegisterAccessor<T>> createCopyDecorator(
      boost::shared_ptr<ChimeraTK::NDRegisterAccessor<T>> target) {
    return boost::make_shared<CopyRegisterDecorator<T>>(target);
  }

} // namespace ChimeraTK::detail

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
