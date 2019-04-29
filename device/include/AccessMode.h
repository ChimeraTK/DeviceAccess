/*
 * AccessMode.h
 *
 *  Created on: May 11, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_ACCESS_MODE_H
#define CHIMERA_TK_ACCESS_MODE_H

#include <map>
#include <set>
#include <sstream>
#include <vector>

#include "Exception.h"

namespace ChimeraTK {

  /** Enum type with access mode flags for register accessors.
   *
   *  Developers note: when adding new flags, also add the flag in the map of the
   * AccessModeFlags with a string representation. */
  enum class AccessMode {

    /** Raw access: disable any possible conversion from the original hardware
     * data type into the given UserType. Obtaining the accessor with a UserType
     * unequal to the actual raw data type will fail and throw a DeviceException
     * with the id EX_WRONG_PARAMETER.
     *
     *  Note: using this flag will make your code intrinsically dependent on the
     * backend type, since the actual raw data type must be known. */
    raw,

    /** Make any read blocking until new data has arrived since the last read.
     * This flag may not be suppoerted by all registers (and backends), in which
     * case a DeviceException with the id NOT_IMPLEMENTED will be thrown. */
    wait_for_new_data

    /* IMPORTANT: When extending this class with new flags, don't forget to update AccessModeFlags::getStringMap()! */
  };

  /** Set of AccessMode flags with additional functionality for an easier
   * handling. */
  class AccessModeFlags {
   public:
    /** Constructor initialises from a std::set<AccessMode> */
    AccessModeFlags(const std::set<AccessMode>& flags) : _flags(flags) {}

    /** Constructor initialises from a brace initialiser list (e.g.
     * "{AccessMode::raw}"). Hint: You can use the brace initialiser list also
     * without explicitly using the class name, when calling a function which has
     *  an argument of the type AccessModeFlags. */
    AccessModeFlags(const std::initializer_list<AccessMode>& flags) : _flags(flags) {}

    /** Check if a certain flag is in the set */
    bool has(AccessMode flag) const { return (_flags.count(flag) != 0); }

    /** Check if the set is empty (i.e. no flag has been set) */
    bool empty() const { return (_flags == std::set<AccessMode>()); }

    /** Check of any flag which is not in the given set "knownFlags" is set. If an
      * unknown flag has been found, a ChimeraTK::logic_error is raised. */
    void checkForUnknownFlags(const std::set<AccessMode>& knownFlags) const {
      for(auto flag : _flags) {
        if(knownFlags.count(flag) == 0) {
          throw ChimeraTK::logic_error("Access mode flag '" + getString(flag) + "' is not known by this backend.");
        }
      }
    }

    /** Check whether two sets of acces mode flags are the same.
       */
    bool operator==(const AccessModeFlags& other) const {
      // fortunately the std::set already has a comparison operator which does exacty what we want
      return _flags == other._flags;
    }

    /** Remove the given flag from the set */
    void remove(const AccessMode flag) { _flags.erase(flag); }

    /** Add the given flag to the set */
    void add(const AccessMode flag) { _flags.insert(flag); }

    /** Get a comma seperated list of all flag strings contained in the class */
    std::string serialize() {
      std::string list{};
      for (auto &f : _flags) {
        list += getString(f) + ",";
      }
      if(!list.empty()){
        list.pop_back(); // remove trailing ','
      }
      return list;
    };

    /** Get a string representation of the given flag */
    static const std::string& getString(const AccessMode flag) { return getStringMap().at(flag); }

    /** Get an AcessModeFlags object from a comma seperated list of flag strings */
    static const AccessModeFlags deserialize(std::string listOfflags){
        std::vector<std::string> names = split(listOfflags);
        std::set<AccessMode> flagList;
        for(auto flagName: names){
            flagList.insert(getAccessMode(flagName));
        }
        return {flagList};
    };

   private:
    /* set of flags */
    std::set<AccessMode> _flags;

    /** return the string map */
    static const std::map<AccessMode, std::string>& getStringMap() {
      static std::map<AccessMode, std::string> m = {
          {AccessMode::raw, "raw"},
          {AccessMode::wait_for_new_data, "wait_for_new_data"}};
      return m;
    }

    static AccessMode getAccessMode(const std::string& flagName) {
      static std::map<std::string, AccessMode> reverse_m;
      for (auto &m : getStringMap()) {
        reverse_m[m.second] = m.first;
      }
      try {
        return reverse_m.at(flagName);
      } catch (std::out_of_range &e){
          throw logic_error("Unknown flag string: " + flagName);
      }
    }

    static std::vector<std::string> split(const std::string &s){
        std::vector<std::string> list;
        std::string tmp;
        char delimiter = ',';

        std::istringstream stream(s);
        while(getline(stream, tmp, delimiter)){
            list.push_back(tmp);
        }
        return list;
    }
  };

} /* namespace ChimeraTK */

#endif /* CHIMERA_TK_ACCESS_MODE_H */
