#pragma once

#include <map>
#include <string>

namespace ChimeraTK {

  /**
   */
  class MetadataCatalogue {
    /** Get metadata information for the given key. */
    [[nodiscard]] const std::string& getMetadata(const std::string& key) const;

    /** Get number of metadata entries in the catalogue */
    [[nodiscard]] size_t getNumberOfMetadata() const;

    /** Add metadata information to the catalogue. Metadata is stored as a
     * key=value pair of strings. */
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
