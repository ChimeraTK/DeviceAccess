#include "VersionNumber.h"

namespace ChimeraTK {
  std::atomic<uint64_t> VersionNumber::_lastGeneratedVersionNumber{0};
}
