// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "TriggeredPollDistributor.h"

namespace ChimeraTK {

  TriggeredPollDistributor::TriggeredPollDistributor(boost::shared_ptr<DeviceBackend> backend,
      boost::shared_ptr<TriggerDistributor<std::nullptr_t>> parent, boost::shared_ptr<AsyncDomain> asyncDomain)
  : SourceTypedAsyncAccessorManager<std::nullptr_t>(std::move(backend), std::move(asyncDomain)),
    _parent(std::move(parent)) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(createAsyncVariable);
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
    return true;
  }

} // namespace ChimeraTK
