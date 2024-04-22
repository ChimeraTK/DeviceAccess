// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

/*
 * Put an extern "C" declaration when compiling with C++. Like this the structs
 * can be used from the included header files. Having this declatation in the
 * header saves extern "C" declaration in all C++ files using this header (avoid
 * code duplication and frogetting the declaration).
 */
#ifdef __cplusplus
extern "C" {
#endif

#define LLRFDRV_IOC '0'
#define LLRFDRV_PHYSICAL_SLOT _IOWR(LLRFDRV_IOC, 20, int)
#define LLRFDRV_DRIVER_VERSION _IOWR(LLRFDRV_IOC, 21, int)
#define LLRFDRV_FIRMWARE_VERSION _IOWR(LLRFDRV_IOC, 22, int)

#define LLRFDRV_IOC_MAXNR 22
#define LLRFDRV_IOC_MINNR 20

#ifdef __cplusplus
} /* closing the extern "C" { */
#endif
