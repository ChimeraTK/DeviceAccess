#ifndef CHIMERATK_SHARED_DUMMY_BACKEND_EXCEPTION_H
#define CHIMERATK_SHARED_DUMMY_BACKEND_EXCEPTION_H

#include <string>

#include "DeviceBackendException.h"

namespace ChimeraTK {

  /// Provides class for exceptions related to SharedDummyDevice
  class SharedDummyBackendException : public DeviceBackendException {
    public:
      enum {
        EX_MAX_NUMBER_OF_MEMBERS_REACHED,
        EX_BAD_ALLOC
      };
      SharedDummyBackendException(const std::string &_exMessage, unsigned int _exID)
        : DeviceBackendException(_exMessage, _exID) {
      }
  };

} // namespace ChimeraTK

#endif /* CHIMERATK_SHARED_DUMMY_BACKEND_EXCEPTION_H */
