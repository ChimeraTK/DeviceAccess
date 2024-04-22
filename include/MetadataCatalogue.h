// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <map>
#include <string>

namespace ChimeraTK {

  /**
   * Container for backend metadata. Metadata is additional information provided by the backend. It can come e.g. from
   * the map file (if existent) and/or may provide e.g. information about the firmware version of the device etc. Which
   * metadata is provided is backend specific, so applications should not rely on the presence of certain metadata.
   *
   * The metadata is a list of key-value pairs with string data types for both key and value.
   */
  class MetadataCatalogue {
   public:
    /** Get metadata information for the given key. If the key is not known, a ChimeraTK::logic_error is thrown. */
    [[nodiscard]] const std::string& getMetadata(const std::string& key) const;

    /** Get number of metadata entries in the catalogue */
    [[nodiscard]] size_t getNumberOfMetadata() const;

    /**
     *  Add metadata information to the catalogue. Metadata is stored as a key=value pair of strings. If the key is
     *  already present, previous information is overwritten.
     */
    void addMetadata(const std::string& key, const std::string& value);

    /** Iterators for meta data */
    using iterator = std::map<std::string, std::string>::iterator;
    using const_iterator = std::map<std::string, std::string>::const_iterator;
    [[nodiscard]] iterator begin();
    [[nodiscard]] const_iterator cbegin() const;
    [[nodiscard]] iterator end();
    [[nodiscard]] const_iterator cend() const;

   protected:
    /** Map of meta data */
    std::map<std::string, std::string> metadata;
  };

} // namespace ChimeraTK
