// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "TransferElementAbstractor.h"
#include "VersionNumber.h"

#include <unordered_set>

namespace ChimeraTK {

  /********************************************************************************************************************/

  /**
   * Group several registers (= TransferElement) which ensures data consistency across multiple variables through an
   * algorithm which matches the VersionNumber. This group does not read on its own. It should work together with a
   * ReadAnyGroup. You should wait for changed variable and transfer it to this group by calling
   * ChimeraTK::DataConsistencyGroup::update. If a consistent state is reached, this function returns true.
   */
  class DataConsistencyGroup {
   public:
    /** Construct empty group. Elements can later be added using the add() function. */
    DataConsistencyGroup();

    /** Construct this group with elements from the list using the add() function. */
    DataConsistencyGroup(std::initializer_list<TransferElementAbstractor> list);
    DataConsistencyGroup(std::initializer_list<boost::shared_ptr<TransferElement>> list);

    template<typename ITERATOR>
    DataConsistencyGroup(ITERATOR first, ITERATOR last);

    /** Add register to group. The same TransferElement can be part of multiple DataConsistencyGroups. The register
     *  must be must be readable, and it must have AccessMode::wait_for_new_data. */
    void add(const TransferElementAbstractor& element);
    void add(boost::shared_ptr<TransferElement> element);

    /** This function updates consistentElements, a set of TransferElementID. It returns true, if a consistent state is
     *  reached. It returns false if an TransferElementID was updated, that was not added to this Group. */
    bool update(const TransferElementID& transferelementid);

    /** Enum describing the matching mode of a DataConsistencyGroup. */
    enum class MatchingMode {
      none, ///< No matching, effectively disable the DataConsitencyGroup. update() will always return true.
      exact ///< Require an exact match of the VersionNumber of all current values of the group's members.
    };

    /** Change the matching mode. The default mode is MatchingMode::exact. */
    void setMatchingMode(MatchingMode newMode) { mode = newMode; }

    /** Return the current matching mode. */
    MatchingMode getMatchingMode() const { return mode; };

    /** For inspection of contents */
    const std::map<TransferElementID, TransferElementAbstractor>& getElements() const { return push_elements; }

    /** returns true if consistent state is reached */
    bool isConsistent() const { return consistentElements.size() == push_elements.size(); }

   private:
    /// A set of TransferElementID, that were updated with update();
    std::unordered_set<TransferElementID> consistentElements;

    std::unordered_set<TransferElementID> lasteSateOfConsistentElements;

    /// Holds the version number this group elements should be consistent to
    VersionNumber versionNumberToBeConsistentTo{nullptr};

    /// Vector of push-type elements in this group, there are only push type elemenst, like in ReadAnyGroup
    std::map<TransferElementID, TransferElementAbstractor> push_elements;

    /// the matching mode used in update()
    MatchingMode mode{MatchingMode::exact};
  };

  /********************************************************************************************************************/

  template<typename ITERATOR>
  DataConsistencyGroup::DataConsistencyGroup(ITERATOR first, ITERATOR last) {
    for(auto it = first; it != last; ++it) add(*it);
  }

  /********************************************************************************************************************/

  inline bool DataConsistencyGroup::update(const TransferElementID& transferElementID) {
    // ignore transferElementID does not belong to DataConsistencyGroup
    if(push_elements.find(transferElementID) == push_elements.end()) {
      return false;
    }
    // if matching mode is none, always return true
    if(mode == MatchingMode::none) return true;
    assert(mode == MatchingMode::exact);

    auto getVNFromElement = push_elements[transferElementID].getVersionNumber();
    assert(getVNFromElement != VersionNumber{nullptr});
    if(getVNFromElement < versionNumberToBeConsistentTo) {
      return false;
    }
    if(versionNumberToBeConsistentTo != getVNFromElement) {
      versionNumberToBeConsistentTo = getVNFromElement;
      consistentElements.clear();
    }
    consistentElements.insert(transferElementID);
    if(consistentElements.size() == push_elements.size()) {
      lasteSateOfConsistentElements = consistentElements;
      return true;
    }
    return false;
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
