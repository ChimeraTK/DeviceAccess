#include "SupportedUserTypes.h"

#include <sstream>

namespace ChimeraTK {

  /********************************************************************************************************************/

template<typename NUMERIC>
std::string detail::numericToUserType_impl<std::string, NUMERIC>::impl(NUMERIC value) {
  return std::to_string(value);
}

/********************************************************************************************************************/

template<typename NUMERIC>
NUMERIC detail::userTypeToNumeric_impl<std::string, NUMERIC>::impl(const std::string& value) {
  NUMERIC v;
  std::stringstream ss(value);
  ss >> v;
  return v;
}

/********************************************************************************************************************/

}
