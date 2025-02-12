// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "BackendRegisterCatalogue.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  [[nodiscard]] std::shared_ptr<async::DataConsistencyRealm> BackendRegisterCatalogueBase::getDataConsistencyRealm(
      const std::vector<size_t>& /* qualifiedAsyncDomainId */) const {
    return nullptr;
  }

  /********************************************************************************************************************/

  [[nodiscard]] RegisterPath BackendRegisterCatalogueBase::getDataConsistencyKeyRegisterPath(
      const std::vector<size_t>& /* qualifiedAsyncDomainId */) const {
    return {};
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
