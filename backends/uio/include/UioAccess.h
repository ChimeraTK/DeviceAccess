// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

// The mainline kernel linux define the maximum number of UIO maps as 5
#ifndef MAX_UIO_MAPS
#  define MAX_UIO_MAPS 5
#endif

#include <boost/filesystem.hpp>

#include <array>
#include <atomic>
#include <memory>
#include <string>
#include <utility>

namespace ChimeraTK {
  /// @brief Implements a generic userspace interface for UIO devices.
  class UioAccess {
   private:
    /// @brief Implements map interface for UIO devices .
    class UioMap {
     public:
      UioMap();
      UioMap(int deviceFileDescriptor, size_t uioMapIdx, const std::string& uioMapPath);
      ~UioMap();
      UioMap(const UioMap&) = delete;
      UioMap& operator=(const UioMap&) = delete;
      UioMap(UioMap&& other) noexcept;
      UioMap& operator=(UioMap&&) noexcept;

      explicit operator bool() const noexcept;

      /// @brief Read data from the specified memory offset address. The address range starts at '0'.
      /// @param address Start address of memory to read from
      /// @param data Address pointer to which data is to be copied
      /// @param sizeInBytes Number of bytes to copy
      void read(uint64_t address, int32_t* data, size_t sizeInBytes);

      /// @brief Write data to the specified memory offset address. The address range starts at '0'.
      /// @param address Start address of memory to write to
      /// @param data Address pointer from which data is to be copied
      /// @param sizeInBytes Number of bytes to copy
      void write(uint64_t address, int32_t const* data, size_t sizeInBytes);

      /// @brief Calculate the address from the perspective of the UIO map
      /// @param address Start address of memory of the request
      /// @param sizeInBytes Number of bytes to copy
      /// @param isWrite Determines if it is a read or a write request
      /// @return Unsigned value
      size_t checkMapAddress(uint64_t address, size_t sizeInBytes, bool isWrite);

     private:
      size_t _deviceLowerBound = 0;
      size_t _deviceHigherBound = 0;
      void* _deviceUserBase = nullptr;
    };

    int _deviceFileDescriptor = 0;
    boost::filesystem::path _deviceFilePath;
    std::string _filename;
    std::array<UioMap, MAX_UIO_MAPS> _maps;
    uint32_t _lastInterruptCount = 0;
    std::atomic<bool> _opened{false};
    uint8_t _maps_number = 0;

    /// @brief Subtracts uint32_t values taking overflow into account.
    /// @param minuend Minuend of subtraction
    /// @param subtrahend Subtrahend of subtraction
    /// @return Result value
    uint32_t subtractUint32OverflowSafe(uint32_t minuend, uint32_t subtrahend);

    /// @brief Reads a decimal formatted value from a file as unsigned 32-bit integer.
    /// @param fileName File path to read from
    /// @return Unsigned value
    static uint32_t readUint32FromFile(std::string fileName);

    /// @brief Reads a hexadecimal formatted value from a file as unsigned 64-bit integer.
    /// @param fileName File path to read from
    /// @return Unsigned value
    static uint64_t readUint64HexFromFile(std::string fileName);

    /// @brief Get an UIO map handle
    /// @param map The map to open
    /// @return Reference to the opened map
    UioAccess::UioMap& getMap(size_t map);

   public:
    explicit UioAccess(const std::string& deviceFilePath);
    ~UioAccess();

    /// @brief Opens UIO device for read and write operations and interrupt handling.
    void open();

    /// @brief Closes UIO device.
    void close();

    // @brief check whether the passed map is valid
    bool mapIndexValid(uint64_t map);

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
