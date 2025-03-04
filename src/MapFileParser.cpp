// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "MapFileParser.h"

#include "JsonMapFileParser.h"
#include "TraditionalMapFileParser.h"

#include <boost/algorithm/string/predicate.hpp>

namespace ChimeraTK {

  /********************************************************************************************************************/

  std::pair<NumericAddressedRegisterCatalogue, MetadataCatalogue> MapFileParser::parse(const std::string& fileName) {
    std::ifstream file;

    file.open(fileName.c_str());
    if(!file) {
      throw ChimeraTK::logic_error("Cannot open file \"" + fileName + "\"");
    }

    if(boost::ends_with(fileName, ".jmap")) {
      detail::JsonMapFileParser parser(fileName);
      return parser.parse(file);
    }
    detail::TraditionalMapFileParser parser(fileName);
    return parser.parse(file);
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
