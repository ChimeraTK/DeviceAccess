// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

// This is C code from the kernel driver. Turn off the C++ linter warnings
// NOLINTBEGIN

#include <sys/types.h>

/** Information about the offsets of the bars in the address space of the
 * character device.
 */
static const loff_t PCIEUNI_BAR_OFFSETS[6] = {0LL, (1LL) << 60, (2LL) << 60, (3LL) << 60, (4LL) << 60, (5LL) << 60};

/** Information about the bar sizes. It is retrieved via IOCTL.
 */
typedef struct _pcieuni_ioctl_bar_sizes {
  size_t barSizes[6]; /** Sizes of bar 0 to 5*/
  size_t dmaAreaSize; /** Size of the address range which can transferred via
                         DMA.*/
} pcieuni_ioctl_bar_sizes;

/* Use 'U' like pcieUni as magic number */
#define PCIEUNI_IOC 'U'
/* relative to the new IOC we keep the same ioctls */
#define PCIEUNI_PHYSICAL_SLOT _IOWR(PCIEUNI_IOC, 60, int)
#define PCIEUNI_DRIVER_VERSION _IOWR(PCIEUNI_IOC, 61, int)
#define PCIEUNI_FIRMWARE_VERSION _IOWR(PCIEUNI_IOC, 62, int)
#define PCIEUNI_GET_DMA_TIME _IOWR(PCIEUNI_IOC, 70, int)
#define PCIEUNI_WRITE_DMA _IOWR(PCIEUNI_IOC, 71, int)
#define PCIEUNI_READ_DMA _IOWR(PCIEUNI_IOC, 72, int)
#define PCIEUNI_SET_IRQ _IOWR(PCIEUNI_IOC, 73, int)
#define PCIEUNI_IOC_MINNR 60
#define PCIEUNI_IOC_MAXNR 63
#define PCIEUNI_IOC_DMA_MINNR 70
#define PCIEUNI_IOC_DMA_MAXNR 74

// NOLINTEND
