// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "TransferElementAbstractor.h"
#include "TransferElementID.h"
#include "VersionNumber.h"

#include <map>
#include <unordered_set>

namespace ChimeraTK::DataConsistencyGroupDetail {

  /********************************************************************************************************************/

  /// Base class for matcher implementations; will not be instantiated directly
  class MatcherBase {
   public:
    virtual ~MatcherBase() = default;

    /** For inspection of contents */
    [[nodiscard]] const auto& getElements() const { return _pushElements; }
    [[nodiscard]] auto& getElements() { return _pushElements; }

   protected:
    /// map of push-type elements in this group, there are only push type elements, like in ReadAnyGroup
    std::map<TransferElementID, TransferElementAbstractor> _pushElements;
  };

  /**
   * Simple matcher implementation for DataConsistencyGroup in MatchingMode::exact.
   *
   * Group several registers (= TransferElement) which ensures data consistency across multiple variables through an
   * algorithm which matches the VersionNumber. This group does not read on its own. It should work together with a
   * ReadAnyGroup. You should wait for changed variable and transfer it to this group by calling
   * ChimeraTK::DataConsistencyGroup::update. If a consistent state is reached, this function returns true.
   */
  class SimpleMatcher : public DataConsistencyGroupDetail::MatcherBase {
   public:
    /** Construct empty group. Elements can later be added using the add() function. */
    SimpleMatcher() = default;

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
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK::DataConsistencyGroupDetail
