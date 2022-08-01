// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "BackendRegisterCatalogue.h"
#include "BackendRegisterInfoBase.h"
#include "ForwardDeclarations.h"
#include "RegisterInfo.h"
#include "TransferElement.h"

#include <boost/shared_ptr.hpp>

#include <mutex>

namespace ChimeraTK {

  struct LNMVariable {
    /** Hold values of CONSTANT or VARIABLE types in a type-dependent table. Only the entry matching the valueType
     *  is actually valid.
     *  Note: We cannot directly put the std::vector in a TemplateUserTypeMap, since it has more than one template
     *  arguments (with defaults). */
    template<typename T>
    struct ValueTable {
      std::vector<T> latestValue;
      DataValidity latestValidity;
      VersionNumber latestVersion;
      struct QueuedValue {
        std::vector<T> value;
        DataValidity validity;
        VersionNumber version;
      };
      std::map<TransferElementID, cppext::future_queue<QueuedValue>> subscriptions;
    };
    TemplateUserTypeMap<ValueTable> valueTable;

    /** Mutex one needs to hold while accessing valueTable. */
    std::mutex valueTable_mutex;
  };

} /* namespace ChimeraTK */
