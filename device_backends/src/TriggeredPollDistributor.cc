// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "TriggeredPollDistributor.h"

#include "TriggerDistributor.h"

namespace ChimeraTK {

  TriggeredPollDistributor::TriggeredPollDistributor(boost::shared_ptr<DeviceBackend> backend,
      std::vector<uint32_t> interruptID, boost::shared_ptr<TriggerDistributor> parent,
      boost::shared_ptr<AsyncDomain> asyncDomain)
  : AsyncAccessorManager(std::move(backend), std::move(asyncDomain)), _id(std::move(interruptID)),
    _parent(std::move(parent)) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(createAsyncVariable);
  }

  //*********************************************************************************************************************/
  void TriggeredPollDistributor::trigger(VersionNumber version) {
    if(!_asyncDomain->_isActive) {
      return;
    }

    _version = version;
    try {
      _transferGroup->read();

      for(auto& var : _asyncVariables) {
        var.second->fillSendBuffer();
        var.second->send(); // send function from  the AsyncVariable base class
      }
    }
    catch(ChimeraTK::runtime_error&) {
      // Nothing to do. Backend's set exception has already been called by the accessor in the transfer group that
      // raised it.
    }
  }

  //*********************************************************************************************************************/
  void TriggeredPollDistributor::activate(VersionNumber version) {
    trigger(version);
  }

} // namespace ChimeraTK
