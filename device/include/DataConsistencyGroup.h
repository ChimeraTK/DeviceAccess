/*
 * DataConsistencyGroup.h
 *
 * Created on: Mar 15, 2019
 *     Author: Jan Timm
 */

#ifndef CHIMERATK_DATAC_ONSISTENCY_GROUP_H
#define CHIMERATK_DATAC_ONSISTENCY_GROUP_H

#include <unordered_set>
#include "TransferElementAbstractor.h"
#include "VersionNumber.h"
#include <iostream> //weg damit

namespace ChimeraTK {
  /** Group several registers (= TransferElement) which ensures data consistency across multiple variables through an
   *  algorithm which matches the VersionNumber. This group does not read on its own. It should work together with a
   *  ReadAnyGroup. You should wait for changed variable and transfer it to this group by calling
   *  ChimeraTK::DataConsistencyGroup::update. If a consistent state is reached, this function returns true. */
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
    void add(TransferElementAbstractor element);
    void add(boost::shared_ptr<TransferElement> element);

    /** This function updates consistentElements, a set of TransferElementID. It returns true, if a consistent state is
     *  reached. It returns false if an TransferElementID was updated, that was not added to this Group. */
    bool update(TransferElementID transferelementid);

   private:
    /// A set of TransferElementID, that were updatet with update();
    std::unordered_set<TransferElementID> consistentElements;

    std::unordered_set<TransferElementID> lasteSateOfConsistentElements;

    /// Holds the version number this group elements should be consistent to
    VersionNumber versionNumberToBeConsistentTo;

    /// Vector of push-type elements in this group, there are only push type elemenst, like in ReadAnyGroup
    std::map<TransferElementID, TransferElementAbstractor> push_elements;
  };

  /********************************************************************************************************************/

  inline DataConsistencyGroup::DataConsistencyGroup() {}

  /********************************************************************************************************************/

  inline DataConsistencyGroup::DataConsistencyGroup(std::initializer_list<TransferElementAbstractor> list) {
    for(auto& element : list) add(element);
  }

  /********************************************************************************************************************/

  inline DataConsistencyGroup::DataConsistencyGroup(std::initializer_list<boost::shared_ptr<TransferElement>> list) {
    for(auto& element : list) add(element);
  }

  /********************************************************************************************************************/

  template<typename ITERATOR>
  DataConsistencyGroup::DataConsistencyGroup(ITERATOR first, ITERATOR last) {
    for(ITERATOR it = first; it != last; ++it) add(*it);
  }
  /********************************************************************************************************************/

  inline void DataConsistencyGroup::add(TransferElementAbstractor element) {
    if(!element.isReadable()) {
      throw ChimeraTK::logic_error(
          "Cannot add non-readable accessor for register " + element.getName() + " to DataConsistencyGroup.");
    }
    if(!element.getAccessModeFlags().has(AccessMode::wait_for_new_data)) {
      throw ChimeraTK::logic_error(
          "Cannot add poll type accessor for register " + element.getName() + " to DataConsistencyGroup.");
    }
    push_elements[element.getId()] = element;
  }

  /********************************************************************************************************************/

  inline void DataConsistencyGroup::add(boost::shared_ptr<TransferElement> element) {
    add(TransferElementAbstractor(element));
  }

  /********************************************************************************************************************/

  inline bool DataConsistencyGroup::update(TransferElementID transferElementID) {
    // ignore transferElementID does not belong to DataConsistencyGroup
    if(push_elements.find(transferElementID) == push_elements.end()) {
      return false;
    }
    auto getVNFromElemenst = push_elements[transferElementID].getVersionNumber();
    if(versionNumberToBeConsistentTo != getVNFromElemenst) {
      versionNumberToBeConsistentTo = getVNFromElemenst;
      consistentElements.clear();
    }
    consistentElements.insert(transferElementID);
    if(consistentElements.size() == push_elements.size()) {
      lasteSateOfConsistentElements = consistentElements;
      return true;
    }
    return false;
  }

} // namespace ChimeraTK
#endif
