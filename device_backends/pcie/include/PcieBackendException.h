#ifndef CHIMERA_TK_PCIE_BACKEND_EXCEPTION_H
#define CHIMERA_TK_PCIE_BACKEND_EXCEPTION_H

#include "DeviceBackend.h"

namespace ChimeraTK {

  /** A class to provide the exceptions for Pcie device."
   *
   */
  class PcieBackendException : public DeviceBackendException {
    public:
      enum {EX_CANNOT_OPEN_DEVICE, EX_DEVICE_OPENED, EX_DEVICE_CLOSED, EX_READ_ERROR, EX_WRITE_ERROR,
        EX_DMA_READ_ERROR, EX_DMA_WRITE_ERROR, EX_INFO_READ_ERROR, EX_UNSUPPORTED_DRIVER};
    public:
      PcieBackendException(const std::string &_exMessage, unsigned int _exID);
    private:

  };

}//namespace ChimeraTK

#endif  /* CHIMERA_TK_PCIE_BACKEND_EXCEPTION_H */

