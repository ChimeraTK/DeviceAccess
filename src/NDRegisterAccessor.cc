// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "NDRegisterAccessor.h"

#include "CopyRegisterDecorator.h"

namespace ChimeraTK {

  template<typename UserType>
  boost::shared_ptr<TransferElement> NDRegisterAccessor<UserType>::makeCopyRegisterDecorator() {
    auto casted = boost::static_pointer_cast<NDRegisterAccessor<UserType>>(enable_shared_from_this::shared_from_this());
    return boost::make_shared<CopyRegisterDecorator<UserType>>(casted);
  }
  /********************************************************************************************************************/

  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(NDRegisterAccessor);

} /* namespace ChimeraTK */
