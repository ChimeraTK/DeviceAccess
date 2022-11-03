// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <map>
#include <set>
#include <sstream>
#include <vector>

namespace ChimeraTK {

  /********************************************************************************************************************/

  /**
   * Enum type with access mode flags for register accessors.
   *
   * Developers note: when adding new flags, also add the flag in the map of the AccessModeFlags with a string
   * representation.
   */
  enum class AccessMode {

    /**
     * Raw access: disable any possible conversion from the original hardware data type into the given UserType.
     * Obtaining the accessor with a UserType unequal to the actual raw data type will fail and throw a DeviceException
     * with the id EX_WRONG_PARAMETER.
     *
     * Note: using this flag will make your code intrinsically dependent on the backend type, since the actual raw data
     * type must be known.
     */
    raw,

    /**
     * Make any read blocking until new data has arrived since the last read. This flag may not be suppoerted by all
     * registers (and backends), in which case a DeviceException with the id NOT_IMPLEMENTED will be thrown.
     */
    wait_for_new_data

    /* IMPORTANT: When extending this class with new flags, don't forget to update AccessModeFlags::getStringMap()! */
  };

  /********************************************************************************************************************/

  /**
   * Set of AccessMode flags with additional functionality for an easier handling.
   *
   * The set holds flags which are enabled for an accessor. The method has() can be used to check wheater a flag is set.
   */
  class AccessModeFlags {
   public:
    /** Constructor initialises from a std::set<AccessMode> */
    explicit AccessModeFlags(std::set<AccessMode> flags);

    /**
     * Constructor initialises from a brace initialiser list (e.g. "{AccessMode::raw}"). Hint: You can use the brace
     * nitialiser list also without explicitly using the class name, when calling a function which has an argument of
     * the type AccessModeFlags.
     */
    AccessModeFlags(const std::initializer_list<AccessMode>& flags);

    AccessModeFlags() = default;

    /**
     *  Check if a certain flag is in the set
     */
    [[nodiscard]] bool has(AccessMode flag) const;

    /**
     *  Check if the set is empty (i.e. no flag has been set)
     */
    [[nodiscard]] bool empty() const;

    /**
     *  Check of any flag which is not in the given set "knownFlags" is set. If an
     * unknown flag has been found, a ChimeraTK::logic_error is raised.
     */
    void checkForUnknownFlags(const std::set<AccessMode>& knownFlags) const;

    /**
     *  Check whether two sets of acces mode flags are the same.
     */
    [[nodiscard]] bool operator==(const AccessModeFlags& other) const;

    /**
     *  "Less than" operator, e.g. for use as key in std::map
     */
    [[nodiscard]] bool operator<(const AccessModeFlags& other) const;

    /**
     *  Remove the given flag from the set
     */
    void remove(AccessMode flag);

    /**
     *  Add the given flag to the set
     */
    void add(AccessMode flag);

    /**
     *  Get a comma seperated list of all flag strings contained in the class
     */
    [[nodiscard]] std::string serialize() const;

    /**
     *  Get a string representation of the given flag
     */
    [[nodiscard]] static const std::string& getString(AccessMode flag);

    /**
     *  Get an AcessModeFlags object from a comma seperated list of flag strings
     */
    [[nodiscard]] static AccessModeFlags deserialize(const std::string& listOfflags);

   private:
    // set of flags
    std::set<AccessMode> _flags;

    /**
     * return the string map
     */
    [[nodiscard]] static const std::map<AccessMode, std::string>& getStringMap();

    /**
     * Convert from string to flag
     */
    [[nodiscard]] static AccessMode getAccessMode(const std::string& flagName);

    /**
     * Split string helper function
     */
    [[nodiscard]] static std::vector<std::string> split(const std::string& s);
  };

  /********************************************************************************************************************/

} /* namespace ChimeraTK */
