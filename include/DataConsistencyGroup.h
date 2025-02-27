// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "TransferElementAbstractor.h"
#include "VersionNumber.h"
#include <unordered_set>

namespace ChimeraTK {

  /********************************************************************************************************************/

  // TODO API merge of DataConsistencyGroup and HDataConsistencyGroup.
  // plan:
  // keep all constructors.
  // but mark some of the constructors/ add functions as deprecated, since they will be incompatible with new implementation
  // keep both implementations under the hood.
  // new add function has non-const TransferElementAbstractor. default histLen = 2.
  //
  // get rid of setMatchingMode; instlead, MatchingMode must be set in constructor. default=exact.
  // if MatchingMode = historized, use new implementation, otherwise use old one.
  // with MatchingMode = historized, update() always returns true.
  // TODO - first check whether it would be a problem to remove setMatchingMode in DoocsAdapter
  // result: if we get rid of setMatchingMode, we will need a lot of (stupid) changes. Maybe easier way:
  // allow changing MatchingMode non/exact->historized but not back.
  // but also provide constructor with MatchingMode

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

    DataConsistencyGroup* _dg;
  };

  /**
   * Group several registers (= TransferElement) which ensures data consistency across multiple variables through an
   * algorithm which matches the VersionNumber. This group does not read on its own. It should work together with a
   * ReadAnyGroup. You should wait for changed variable and transfer it to this group by calling
   * ChimeraTK::DataConsistencyGroup::update. If a consistent state is reached, this function returns true.
   */
  class DataConsistencyGroup {
    friend class SimpleMatcher;

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
    bool update(const TransferElementID& transferElementID);

    /** Enum describing the matching mode of a DataConsistencyGroup. */
    enum class MatchingMode {
      none, ///< No matching, effectively disable the DataConsitencyGroup. update() will always return true.
      exact ///< Require an exact match of the VersionNumber of all current values of the group's members.
    };

    /** Change the matching mode. The default mode is MatchingMode::exact. */
    void setMatchingMode(MatchingMode newMode) { _mode = newMode; }

    /** Return the current matching mode. */
    [[nodiscard]] MatchingMode getMatchingMode() const { return _mode; };

    /** For inspection of contents */
    [[nodiscard]] const std::map<TransferElementID, TransferElementAbstractor>& getElements() const {
      return _pushElements;
    }

    /** returns true if consistent state is reached */
    [[nodiscard]] bool isConsistent() const { return sImpl->isConsistent(); }

   private:
    /// Holds the version number this group elements should be consistent to
    VersionNumber versionNumberToBeConsistentTo{nullptr}; // TODO check whether needed in outer class

    /// map of push-type elements in this group, there are only push type elemenst, like in ReadAnyGroup
    std::map<TransferElementID, TransferElementAbstractor> _pushElements;

    /// the matching mode used in update()
    MatchingMode _mode{MatchingMode::exact};

    SimpleMatcher* sImpl = nullptr;
  };

  /********************************************************************************************************************/

  template<typename ITERATOR>
  DataConsistencyGroup::DataConsistencyGroup(ITERATOR first, ITERATOR last) {
    for(auto it = first; it != last; ++it) add(*it);
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
