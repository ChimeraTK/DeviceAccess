#pragma once

#include <ChimeraTK/TransferElement.h>

namespace ChimeraTK {

  /** A Helper struct to store an accessor and a version number.
 *  Like this you can set the user buffer of the accessors and the version number which shall be used
 *  when the accessor is written, but delay the writing do a later point in time.
 */
  struct RecoveryHelper {
    boost::shared_ptr<TransferElement> accessor;
    VersionNumber versionNumber;
    RecoveryHelper(boost::shared_ptr<TransferElement> a, VersionNumber v = VersionNumber(nullptr))
    : accessor(a), versionNumber(v) {}
  };

} // end of namespace ChimeraTK
