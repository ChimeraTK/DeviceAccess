// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <cstddef>
#include <functional>
#include <iosfwd>

namespace ChimeraTK {

  /*******************************************************************************************************************/

  /**
   * Simple class holding a unique ID for a TransferElement. The ID is guaranteed
   * to be unique for all accessors throughout the lifetime of the process.
   */
  class TransferElementID {
   public:
    /**
     * Default constructor constructs an invalid ID, which may be assigned with another ID
     */
    TransferElementID() = default;

    /**
     * Copy ID from another
     */
    TransferElementID(const TransferElementID& other) = default;

    /**
     * Compare ID with another. Will always return false, if the ID is invalid (i.e. setId() was never called).
     */
    bool operator==(const TransferElementID& other) const { return (_id != 0) && (_id == other._id); }
    bool operator!=(const TransferElementID& other) const { return !(operator==(other)); }

    /** Check whether the ID is valid. */
    [[nodiscard]] bool isValid() const { return _id != 0; }

    /** Assign ID from another. */
    TransferElementID& operator=(const TransferElementID& other) = default;

   protected:
    /**
     * Assign an ID to this instance. May only be called if currently no ID has been assigned.
     */
    void makeUnique();

    /** The actual ID value */
    size_t _id{0};

    // Hash function for putting TransferElement::ID e.g. into an std::unordered_map
    friend struct std::hash<TransferElementID>;

    // Comparison for putting TransferElement::ID e.g. into an std::map
    friend struct std::less<TransferElementID>;

    friend std::ostream& operator<<(std::ostream& os, const TransferElementID& me);
    friend class TransferElement;
  };

  /*******************************************************************************************************************/

  /**
   * Streaming operator to stream the TransferElementID e.g. to std::cout
   */
  std::ostream& operator<<(std::ostream& os, const TransferElementID& me);

  /*******************************************************************************************************************/

} // namespace ChimeraTK
