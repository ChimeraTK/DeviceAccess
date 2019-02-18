/*
 * TransferElementID.h
 *
 *  Created on: Dec 18, 2017
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_TRANSFER_ELEMENT_ID_H
#define CHIMERA_TK_TRANSFER_ELEMENT_ID_H

#include <ostream>

namespace ChimeraTK {

  /**
   * Simple class holding a unique ID for a TransferElement. The ID is guaranteed to be unique for all accessors
   * throughout the lifetime of the process.
   */
  class TransferElementID {
   public:
    /** Default constructor constructs an invalid ID, which may be assigned with another ID */
    TransferElementID() : _id(0) {}

    /** Copy ID from another */
    TransferElementID(const TransferElementID& other) : _id(other._id) {}

    /** Compare ID with another. Will always return false, if the ID is invalid (i.e. setId() was never called). */
    bool operator==(const TransferElementID& other) const { return (_id != 0) && (_id == other._id); }
    bool operator!=(const TransferElementID& other) const { return !(operator==(other)); }

    /** Assign ID from another. */
    TransferElementID& operator=(const TransferElementID& other) {
      _id = other._id;
      return *this;
    }

    /** Streaming operator to stream the ID e.g. to std::cout */
    friend std::ostream& operator<<(std::ostream& os, const TransferElementID& me) {
      std::stringstream ss;
      ss << std::hex << std::showbase << me._id;
      os << ss.str();
      return os;
    }

    /** Hash function for putting TransferElement::ID e.g. into an std::unordered_map */
    friend struct std::hash<TransferElementID>;

    /** Comparison for putting TransferElement::ID e.g. into an std::map */
    friend struct std::less<TransferElementID>;

   protected:
    /** Assign an ID to this instance. May only be called if currently no ID has been assigned. */
    void makeUnique() {
      static std::atomic<size_t> nextId{0};
      assert(_id == 0);
      ++nextId;
      assert(nextId != 0);
      _id = nextId;
    }

    /** The actual ID value */
    size_t _id;

    friend class TransferElement;
  };

} // namespace ChimeraTK

#endif // CHIMERA_TK_TRANSFER_ELEMENT_ID_H
