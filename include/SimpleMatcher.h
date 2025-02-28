// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "TransferElementID.h"
#include "VersionNumber.h"
#include <unordered_set>

namespace ChimeraTK {

  /********************************************************************************************************************/

  class DataConsistencyGroup;

  /// not for direct use - maybe hide it?
  /**
   * Group several registers (= TransferElement) which ensures data consistency across multiple variables through an
   * algorithm which matches the VersionNumber. This group does not read on its own. It should work together with a
   * ReadAnyGroup. You should wait for changed variable and transfer it to this group by calling
   * ChimeraTK::DataConsistencyGroup::update. If a consistent state is reached, this function returns true.
   */
  class SimpleMatcher {
   public:
    /** Construct empty group. Elements can later be added using the add() function. */
    explicit SimpleMatcher(DataConsistencyGroup* dg) : _dg(dg) {}

    /** This function updates consistentElements, a set of TransferElementID. It returns true, if a consistent state is
     *  reached. It returns false if an TransferElementID was updated, that was not added to this Group. */
    bool update(const TransferElementID& transferElementID);

    /** returns true if consistent state is reached */
    bool isConsistent() const;

   private:
    /// A set of TransferElementID, that were updated with update();
    std::unordered_set<TransferElementID> _consistentElements;
    /// Holds the version number this group elements should be consistent to
    VersionNumber _versionNumberToBeConsistentTo{nullptr};

    DataConsistencyGroup* _dg;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
