#include "VersionNumber.h"

namespace ChimeraTK {
  std::atomic<uint64_t> VersionNumber::_lastGeneratedVersionNumber{0};

  /** Stream operator passing the human readable representation to an ostream. */
  std::ostream& operator<<(std::ostream& stream, const VersionNumber& version) {
    stream << std::string(version);
    return stream;
  }
} /* namespace ChimeraTK */
