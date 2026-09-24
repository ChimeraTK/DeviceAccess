// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "async/TriggeredPollDistributor.h"

#include "async/DataConsistencyKey.h"
#include "async/SubDomain.h"
#include "BackendRegisterCatalogue.h"
#include "NumericAddressedBackend.h"
#include "SelectorGate.h"

namespace ChimeraTK::async {

  /********************************************************************************************************************/

  TriggeredPollDistributor::TriggeredPollDistributor(boost::shared_ptr<DeviceBackend> backend,
      boost::shared_ptr<SubDomain<std::nullptr_t>> parent, boost::shared_ptr<Domain> asyncDomain)
  : SourceTypedAsyncAccessorManager<std::nullptr_t>(std::move(backend), std::move(asyncDomain)),
    _parent(std::move(parent)) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(createAsyncVariable);

    // get DataConsistencyRealm from backend catalogue
    _dataConsistencyRealm = _backend->getRegisterCatalogue().getImpl().getDataConsistencyRealm(_parent->getId());
    if(_dataConsistencyRealm) {
      // get accessor for the DataConsistencyKey from the backend and add it to our TransferGroup
      auto path = _backend->getRegisterCatalogue().getImpl().getDataConsistencyKeyRegisterPath(_parent->getId());
      _dataConsistencyKeyAccessor.replace(_backend->getRegisterAccessor<DataConsistencyKey::BaseType>(path, 0, 0, {}));
      _transferGroup.addAccessor(_dataConsistencyKeyAccessor);
    }
  }

  /********************************************************************************************************************/

  bool TriggeredPollDistributor::prepareIntermediateBuffers() {
    try {
      _transferGroup.read();
    }
    catch(ChimeraTK::runtime_error&) {
      // Nothing to do. Backend's set exception has already been called by the accessor in the transfer group that
      // raised it.
      return false;
    }

    // update version number
    if(_dataConsistencyRealm) {
      auto newVersion = _dataConsistencyRealm->getVersion(DataConsistencyKey(_dataConsistencyKeyAccessor));
      if(newVersion >= _lastVersion) {
        _version = newVersion;
        _lastVersion = _version;
        _forceFaulty = false;
      }
      else {
        _version = _lastVersion;
        _forceFaulty = true;
      }
    }

    return true;
  }

  /********************************************************************************************************************/

  SelectorGate TriggeredPollDistributor::buildSelectorGate(const AccessorInstanceDescriptor& descriptor) {
    // Only numeric-addressed backends carry per-register selection metadata; other backends have no
    // getSelectedBy and remain ungated.
    auto numericAddressed = boost::dynamic_pointer_cast<NumericAddressedBackend>(_backend);
    if(!numericAddressed) {
      return SelectorGate();
    }
    auto selectedBy = _backend->getRegisterCatalogue().getImpl().getSelectedBy(descriptor.name);
    if(!selectedBy) {
      return SelectorGate();
    }
    // forceFirstFaulty=true: a freshly subscribed consumer is reported faulty until the selector
    // register actually matches, so it does not spuriously see valid data before the selection is set.
    SelectorGate gate;
    gate.replace(numericAddressed, *selectedBy, true);
    return gate;
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK::async
