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