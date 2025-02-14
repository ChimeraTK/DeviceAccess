// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "TransferElementAbstractor.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  namespace DataConsistencyGroupDetail {
    class MatcherBase;
  }

  /**
   * Group several registers (= TransferElement) which ensures data consistency across multiple variables through an
   * algorithm which matches the VersionNumber. This group does not read on its own. It should work together with a
   * ReadAnyGroup.
   * This is a proxy class, which delegates to two different implementations.
   * MatchingMode=exact is handled by SimpleMatcher (the legacy DataConsistencyGroup implementation).
   * For this, you should wait for changed variable and transfer it to this group by calling
   * DataConsistencyGroup::update. If a consistent state is reached, this function returns true.
   * MatchingMode=historized is handled by HistorizedMatcher. In this case, the provided accessors are decorated
   * with DataConsistencyDecorators, with the effect that the readAny() returns only on consistent inputs.
   * In this mode, it is unnecessary but still allowed to call DataConsistencyGroup::update and it simply returns true.
   */
  class DataConsistencyGroup {
   public:
    /** Enum describing the matching mode of a DataConsistencyGroup. */
    enum class MatchingMode {
      none,  ///< No matching, effectively disable the DataConsitencyGroup. update() will always return true.
      exact, ///< Require an exact match of the VersionNumber of all current values of the group's members.
             ///< Require an exact match of the VersionNumber of all current or historized values of the group's members
      historized
    };
    constexpr static unsigned defaultHistLen = 2;

    /** Construct empty group. Elements can later be added using the add() function. */
    explicit DataConsistencyGroup(MatchingMode mode = MatchingMode::exact);
    ~DataConsistencyGroup();
    DataConsistencyGroup(const DataConsistencyGroup& other) = delete;
    DataConsistencyGroup(DataConsistencyGroup&& other) noexcept;
    DataConsistencyGroup& operator=(DataConsistencyGroup&& other) noexcept;

    /** Construct this group with elements from the list using the add() function. */
    [[deprecated("use list constructor with MatchingMode instead")]]
    DataConsistencyGroup(std::initializer_list<TransferElementAbstractor> list);
    [[deprecated("use list constructor with MatchingMode instead")]]
    DataConsistencyGroup(std::initializer_list<boost::shared_ptr<TransferElement>> list);
    DataConsistencyGroup(std::initializer_list<std::reference_wrapper<TransferElementAbstractor>> list,
        MatchingMode mode, unsigned histLen = defaultHistLen);

    template<typename ITERATOR>
    DataConsistencyGroup(
        ITERATOR first, ITERATOR last, MatchingMode mode = MatchingMode::exact, unsigned int histLen = defaultHistLen);

    /** Add register to group. The same TransferElement can be part of multiple DataConsistencyGroups. The register
     *  must be must be readable, and it must have AccessMode::wait_for_new_data. */
    [[deprecated("use add function with histLen instead")]]
    void add(const TransferElementAbstractor& element);
    [[deprecated("use add function with histLen instead")]]
    void add(boost::shared_ptr<TransferElement> element);
    /** Add register to group. The same TransferElement can be part of multiple DataConsistencyGroups. The register
     *  must be must be readable, and it must have AccessMode::wait_for_new_data.
     *  This function may modify the register accessor, placing a DataConsistencyDecorator around it.
     **/
    void add(TransferElementAbstractor& acc, unsigned histLen = defaultHistLen);

    /** This function must be called after an update was received from the ReadAnyGroup.
     *  It returns true, if a consistent state is reached.
     *  It returns false if an TransferElementID was updated, that was not added to this group.
     *  For MatchingMode::historized, readAny will only let through consistent updates, so then update always returns
     *  true.
     */
    bool update(const TransferElementID& transferElementID);

    /** Change the matching mode. The default mode is MatchingMode::exact, if not set differently in constructor.
     *  This method is deprecated since it's not possible to set MatchingMode::historized like this.
     */
    [[deprecated("set MatchingMode in constructor instead")]]
    void setMatchingMode(MatchingMode newMode);

    /** Return the current matching mode. */
    [[nodiscard]] MatchingMode getMatchingMode() const { return _mode; };

    /** For inspection of contents */
    [[nodiscard]] const std::map<TransferElementID, TransferElementAbstractor>& getElements() const;
    /** For diagnostics */
    [[nodiscard]] const DataConsistencyGroupDetail::MatcherBase& getMatcher();

    /** returns true if consistent state is reached */
    [[nodiscard]] bool isConsistent() const;

   private:
    void initMatcher();
    static void checkAccess(const TransferElementAbstractor& element);

    /// the matching mode used in update()
    MatchingMode _mode{MatchingMode::exact};

    std::unique_ptr<DataConsistencyGroupDetail::MatcherBase> _impl;
  };

  /********************************************************************************************************************/

  template<typename ITERATOR>
  DataConsistencyGroup::DataConsistencyGroup(ITERATOR first, ITERATOR last, MatchingMode mode, unsigned int histLen)
  : _mode(mode) {
    initMatcher();
    for(auto it = first; it != last; ++it) {
      add(*it, histLen);
    }
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
