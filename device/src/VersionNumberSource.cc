#include "VersionNumberSource.h"

namespace ChimeraTK {
  std::atomic<VersionNumber::UnderlyingDataType> VersionNumberSource::_lastReturnedVersionNumber{0};
}
