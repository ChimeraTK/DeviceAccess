// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "MetadataCatalogue.h"

#include "Exception.h"

#include <stdexcept>

namespace ChimeraTK {

  /********************************************************************************************************************/

  const std::string& MetadataCatalogue::getMetadata(const std::string& key) const {
    try {
      return metadata.at(key);
    }
    catch(std::out_of_range&) {
      throw ChimeraTK::logic_error("");
    }
  }

  /********************************************************************************************************************/

  size_t MetadataCatalogue::getNumberOfMetadata() const {
    return metadata.size();
  }

  /********************************************************************************************************************/

  void MetadataCatalogue::addMetadata(const std::string& key, const std::string& value) {
    metadata[key] = value;
  }

  /********************************************************************************************************************/

  MetadataCatalogue::iterator MetadataCatalogue::begin() {
    return metadata.begin();
  }

  /********************************************************************************************************************/

  MetadataCatalogue::const_iterator MetadataCatalogue::cbegin() const {
    return metadata.cbegin();
  }

  /********************************************************************************************************************/

  MetadataCatalogue::iterator MetadataCatalogue::end() {
    return metadata.end();
  }

  /********************************************************************************************************************/

  MetadataCatalogue::const_iterator MetadataCatalogue::cend() const {
    return metadata.cend();
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
