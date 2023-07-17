// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <filesystem>
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE UioBackendUnifiedTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "MapFileParser.h"
#include "UnifiedBackendTest.h"
#include <sys/file.h>
#include <sys/mman.h>

#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <poll.h>
#include <unistd.h>

using namespace ChimeraTK;

/**********************************************************************************************************************/

// Use a file lock to ensure we are not running concurrent tests in
// parallel using the same kernel dummy driver.
//
// Note: The lock is automatically released when the process terminates!
struct TestLocker {
  const std::string lockfile{"/var/run/lock/uiodummy.lock"};
  TestLocker() {
    int fd = open(lockfile.c_str(), O_WRONLY | O_CREAT, 0777);
    if(fd == -1) {
      exit(1);
    }

    // obtain lock
    int res = flock(fd, LOCK_EX);
    if(res == -1) {
      exit(1);
    }
  }

  ~TestLocker() {
    // FIXME: It would be nice to unlink the file here, so it does not unnecessarily remain in the file system.
    // Unfortunately, this somehow spoils the locking. Completely unclear why.

    // unlink(lockfile.c_str());
  }
};
static TestLocker testLocker;

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE(UioBackendUnifiedTestSuite)

/**********************************************************************************************************************/

static const std::string cdd("(uio:ctkuiodummy?map=uioBackendTest.mapp)");

/**********************************************************************************************************************/

class RawUioAccess {
 public:
  explicit RawUioAccess(const std::string& filePath, const std::string& mapFile);
  ~RawUioAccess();
  void sendInterrupt() const;
  [[nodiscard]] size_t getMemorySize() const;
  void* data();
  template<typename T>
  T read(const std::string& name);
  template<typename T>
  void write(const std::string& name, T value);

 private:
  int _uioFileDescriptor;
  int _uioProcFd;
  std::filesystem::path _deviceFilePath;
  size_t _deviceMemSize = 0;
  void* _memoryPointer{nullptr};
  NumericAddressedRegisterCatalogue _catalogue;

  static uint64_t readUint64HexFromFile(const std::string& filePath);
};

/**********************************************************************************************************************/

RawUioAccess::RawUioAccess(const std::string& filePath, const std::string& mapFile)
: _deviceFilePath(filePath.c_str()) {
  if(std::filesystem::is_symlink(_deviceFilePath)) {
    _deviceFilePath = std::filesystem::canonical(_deviceFilePath);
  }
  _uioFileDescriptor = open(filePath.c_str(), O_RDWR);

  if(_uioFileDescriptor < 0) {
    throw std::runtime_error("failed to open UIO device '" + filePath + "'");
  }

  _uioProcFd = open("/proc/uio-dummy", O_RDWR);
  if(_uioProcFd < 0) {
    throw std::runtime_error("failed to open UIO device '" + filePath + "'");
  }

  // Determine size of UIO memory region
  std::string fileName = _deviceFilePath.filename().string();
  _deviceMemSize = readUint64HexFromFile("/sys/class/uio/" + fileName + "/maps/map0/size");

  _memoryPointer = mmap(nullptr, _deviceMemSize, PROT_READ | PROT_WRITE, MAP_SHARED, _uioFileDescriptor, 0);

  if(_memoryPointer == MAP_FAILED) {
    throw std::runtime_error("UioMmap construction failed");
  }
  MapFileParser p;
  auto [cat, metaCat] = p.parse(mapFile);
  _catalogue = std::move(cat);
}

/**********************************************************************************************************************/
/* Helper implementations                                                                                             */
/**********************************************************************************************************************/

RawUioAccess::~RawUioAccess() {
  munmap(_memoryPointer, _deviceMemSize);
  close(_uioFileDescriptor);
  close(_uioProcFd);
}

/**********************************************************************************************************************/

void RawUioAccess::sendInterrupt() const {
  static int enable{1};
  ::write(_uioProcFd, &enable, sizeof(enable));
}

/**********************************************************************************************************************/

size_t RawUioAccess::getMemorySize() const {
  return _deviceMemSize;
}

/**********************************************************************************************************************/

void* RawUioAccess::data() {
  return _memoryPointer;
}

/**********************************************************************************************************************/

template<typename T>
T RawUioAccess::read(const std::string& name) {
  auto r = _catalogue.getBackendRegister(name);
  return *reinterpret_cast<T*>(reinterpret_cast<std::byte*>(data()) + r.address);
}

/**********************************************************************************************************************/

template<typename T>
void RawUioAccess::write(const std::string& name, T value) {
  auto r = _catalogue.getBackendRegister(name);
  *reinterpret_cast<T*>(reinterpret_cast<std::byte*>(data()) + r.address) = value;
  sendInterrupt();
}

/**********************************************************************************************************************/

uint64_t RawUioAccess::readUint64HexFromFile(const std::string& filePath) {
  uint64_t value = 0;
  std::ifstream inputFile(filePath.c_str());

  if(inputFile.is_open()) {
    inputFile >> std::hex >> value;
    inputFile.close();
  }
  return value;
}

/**********************************************************************************************************************/
/* The test descriptors                                                                                               */
/**********************************************************************************************************************/

template<typename Derived>
struct ScalarDescriptor {
  Derived* derived = static_cast<Derived*>(this);
  AccessModeFlags supportedFlags() { return {AccessMode::raw}; }
  size_t nChannels() { return 1; }
  size_t nElementsPerChannel() { return 1; }
  size_t writeQueueLength() { return std::numeric_limits<size_t>::max(); }
  size_t nRuntimeErrorCases() { return 0; }
  using minimumUserType = int32_t;
  using rawUserType = minimumUserType;

  static constexpr auto capabilities = TestCapabilities<>()
                                           .disableForceDataLossWrite()
                                           .disableSwitchReadOnly()
                                           .disableSwitchWriteOnly()
                                           .disableTestWriteNeverLosesData()
                                           .disableAsyncReadInconsistency()
                                           .enableTestRawTransfer();

  std::shared_ptr<RawUioAccess> remote{new RawUioAccess("/dev/ctkuiodummy", "uioBackendTest.mapp")};

  // Type can be raw type or user type
  template<typename Type>
  std::vector<std::vector<Type>> generateValue(bool raw = false) {
    auto rawVal00 = remote->read<rawUserType>(derived->path());
    rawVal00 += 3;
    Type val00 = (raw ? rawVal00 : rawToCooked<Type, rawUserType>(rawVal00));

    return {{val00}};
  }

  // Type can be raw type or user type
  template<typename Type>
  std::vector<std::vector<Type>> getRemoteValue(bool raw = false) {
    auto rawVal00 = remote->read<rawUserType>(derived->path());
    Type val00 = (raw ? rawVal00 : rawToCooked<Type, rawUserType>(rawVal00));

    return {{val00}};
  }

  void setRemoteValue() {
    auto x = generateValue<rawUserType>(true)[0][0];
    remote->write<rawUserType>(derived->path(), x);
  }

  // default implementation just casting. Re-implement in derived classes if needed.
  template<typename UserType, typename RawType>
  RawType cookedToRaw(UserType val) {
    return static_cast<RawType>(val);
  }

  // default implementation just casting. Re-implement in derived classes if needed.
  template<typename UserType, typename RawType>
  UserType rawToCooked(RawType val) {
    return static_cast<UserType>(val);
  }

  // we need this because it's expected in template, but unused
  void setForceRuntimeError([[maybe_unused]] bool enable, [[maybe_unused]] size_t type) {}
};

/**********************************************************************************************************************/

struct Scalar32 : ScalarDescriptor<Scalar32> {
  static std::string path() { return "TIMING.WORD_ID"; }
  static bool isReadable() { return true; }
  static bool isWriteable() { return false; }
};

/**********************************************************************************************************************/

struct Scalar32Async : ScalarDescriptor<Scalar32Async> {
  static std::string path() { return "MOTOR_CONTROL.MOTOR_POSITION"; }
  static bool isReadable() { return true; }
  static bool isWriteable() { return false; }
  static ChimeraTK::AccessModeFlags supportedFlags() {
    return {ChimeraTK::AccessMode::wait_for_new_data, ChimeraTK::AccessMode::raw};
  }
};

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testUnified) {
  UnifiedBackendTest<>().addRegister<Scalar32>().addRegister<Scalar32Async>().runTests(cdd);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
