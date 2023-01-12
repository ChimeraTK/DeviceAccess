// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <boost/filesystem.hpp>

#include <atomic>
#include <string>

namespace ChimeraTK {
  /// @brief Implements a generic userspace interface for UIO devices.
  class UioAccess {
   private:
    boost::filesystem::path _deviceFilePath;
    int _deviceFileDescriptor = 0;
    void* _deviceUserBase = nullptr;
    void* _deviceKernelBase = nullptr;
    size_t _deviceMemSize = 0;
    uint32_t _lastInterruptCount = 0;
    std::atomic<bool> _opened{false};

    /// @brief Maps user space memory range to address range of UIO device.
    void UioMMap();

    /// @brief Unmaps user space memory range for address range of UIO device.
    void UioUnmap();

    /// @brief Subtracts uint32_t values taking overflow into account.
    /// @param minuend Minuend of subtraction
    /// @param subtrahend Subtrahend of subtraction
    /// @return Result value
    uint32_t subtractUint32OverflowSafe(uint32_t minuend, uint32_t subtrahend);

    /// @brief Reads a decimal formatted value from a file as unsigned 32-bit integer.
    /// @param fileName File path to read from
    /// @return Unsigned value
    uint32_t readUint32FromFile(std::string fileName);

    /// @brief Reads a hexadecimal formatted value from a file as unsigned 64-bit integer.
    /// @param fileName File path to read from
    /// @return Unsigned value
    uint64_t readUint64HexFromFile(std::string fileName);

   public:
    UioAccess(std::string deviceFilePath);
    ~UioAccess();

    /// @brief Opens UIO device for read and write operations and interrupt handling.
    void open();

    /// @brief Closes UIO device.
    void close();

    /// @brief Read data from the specified memory offset address. The address range starts at '0'.
    /// @param map Selected UIO memory region. Only region '0' is currently supported.
    /// @param address Start address of memory to read from
    /// @param data Address pointer to which data is to be copied
    /// @param sizeInBytes Number of bytes to copy
    void read(uint64_t map, uint64_t address, int32_t* data, size_t sizeInBytes);

    /// @brief Write data to the specified memory offset address. The address range starts at '0'.
    /// @param map Selected UIO memory region. Only region '0' is currently supported.
    /// @param address Start address of memory to write to
    /// @param data Address pointer from which data is to be copied
    /// @param sizeInBytes Number of bytes to copy
    void write(uint64_t map, uint64_t address, int32_t const* data, size_t sizeInBytes);

    /// @brief Wait for hardware interrupt to occur within specified timeout period.
    /// @param timeoutMs Timeout period in ms
    /// @return Number of interrupts that occurred. '0' for none withing timeout period.
    uint32_t waitForInterrupt(int timeoutMs);

    /// @brief Clear all pending interrupts.
    void clearInterrupts();

    /// @brief Return UIO device file path.
    /// @return File path
    std::string getDeviceFilePath();
  };
} // namespace ChimeraTK
