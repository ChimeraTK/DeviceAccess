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
    uint64_t writeOrder;
    bool wasWritten{false};

    RecoveryHelper(boost::shared_ptr<TransferElement> a, VersionNumber v = VersionNumber(nullptr), uint64_t order = 0)
    : accessor(a), versionNumber(v), writeOrder(order) {}
  };

} // end of namespace ChimeraTK
