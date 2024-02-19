// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "TransferElement.h"

#include <boost/shared_ptr.hpp>

#include <mutex>

namespace ChimeraTK {

  namespace LNMBackend {
    class MathPlugin;
  }

  struct LNMVariable {
    /** Hold values of CONSTANT or VARIABLE types in a type-dependent table. Only the entry matching the valueType
     *  is actually valid.
     *  Note: We cannot directly put the std::vector in a TemplateUserTypeMap, since it has more than one template
     *  arguments (with defaults). */
    template<typename T>
    struct ValueTable {
      std::vector<T> latestValue;
      DataValidity latestValidity{DataValidity::ok};
      VersionNumber latestVersion{nullptr};
      struct QueuedValue {
        std::vector<T> value;
        DataValidity validity{DataValidity::ok};
        VersionNumber version{nullptr};
      };
      std::map<TransferElementID, cppext::future_queue<QueuedValue>> subscriptions;
    };
    TemplateUserTypeMap<ValueTable> valueTable;

    /** Mutex one needs to hold while accessing valueTable. */
    std::mutex valueTable_mutex;

    /** formulas which need updates after variable was written */
    std::set<LNMBackend::MathPlugin*> usingFormulas;

    /** type of the variable */
    ChimeraTK::DataType valueType;

    /** flag whether this variable is actaully a constant */
    bool isConstant{false};
  };

} /* namespace ChimeraTK */
