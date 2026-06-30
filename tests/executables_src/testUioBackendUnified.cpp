// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <filesystem>
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE UioBackendUnifiedTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "MapFileParser.h"
#include "UnifiedBackendTest.h"

#include <sys/file.h>
#include <sys/mman.h>

#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <vector>

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

  // FIXME: It would be nice to unlink the file here, so it does not unnecessarily remain in the file system.
  // Unfortunately, this somehow spoils the locking. Completely unclear why.
  // unlink(lockfile.c_str());
  ~TestLocker() = default;
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
  template<typename T>
  T read(const std::string& name);
  template<typename T>
  void write(const std::string& name, T value);

 private:
  int _uioFileDescriptor;
  int _uioProcFd;
  std::filesystem::path _deviceFilePath;
  std::vector<size_t> _mapSizes;
  std::vector<void*> _mapPointers;
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

  std::string fileName = _deviceFilePath.filename().string();
  for(size_t mapIdx = 0;; ++mapIdx) {
    std::string mapPath = "/sys/class/uio/" + fileName + "/maps/map" + std::to_string(mapIdx);
    if(!std::filesystem::is_directory(mapPath)) {
      break;
    }
    size_t mapSize = readUint64HexFromFile(mapPath + "/size");
    void* ptr = mmap(nullptr, mapSize, PROT_READ | PROT_WRITE, MAP_SHARED, _uioFileDescriptor,
        static_cast<off_t>(mapIdx * getpagesize()));
    if(ptr == MAP_FAILED) {
      throw std::runtime_error("UioMmap construction failed for map" + std::to_string(mapIdx));
    }
    _mapSizes.push_back(mapSize);
    _mapPointers.push_back(ptr);
  }

  auto [cat, metaCat] = MapFileParser::parse(mapFile);
  _catalogue = std::move(cat);
}

/**********************************************************************************************************************/
/* Helper implementations                                                                                             */
/**********************************************************************************************************************/

RawUioAccess::~RawUioAccess() {
  for(size_t i = 0; i < _mapPointers.size(); ++i) {
    munmap(_mapPointers[i], _mapSizes[i]);
  }
  close(_uioFileDescriptor);
  close(_uioProcFd);
}

/**********************************************************************************************************************/

void RawUioAccess::sendInterrupt() const {
  static int enable{1};
  std::ignore = ::write(_uioProcFd, &enable, sizeof(enable));
}

/**********************************************************************************************************************/

template<typename T>
T RawUioAccess::read(const std::string& name) {
  auto r = _catalogue.getBackendRegister(name);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return *reinterpret_cast<T*>(reinterpret_cast<std::byte*>(_mapPointers[r.bar]) + r.address);
}

/**********************************************************************************************************************/

template<typename T>
void RawUioAccess::write(const std::string& name, T value) {
  auto r = _catalogue.getBackendRegister(name);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  *reinterpret_cast<T*>(reinterpret_cast<std::byte*>(_mapPointers[r.bar]) + r.address) = value;
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

struct Scalar32Map0 : ScalarDescriptor<Scalar32Map0> {
  static std::string path() { return "BSP.SCRATCH"; }
  static bool isReadable() { return true; }
  static bool isWriteable() { return true; }
};

/**********************************************************************************************************************/

struct Scalar32Map1 : ScalarDescriptor<Scalar32Map1> {
  static std::string path() { return "TIMING.WORD_ID"; }
  static bool isReadable() { return true; }
  static bool isWriteable() { return false; }
};

/**********************************************************************************************************************/

struct Scalar32Map1Async : ScalarDescriptor<Scalar32Map1Async> {
  static std::string path() { return "MOTOR_CONTROL.MOTOR_POSITION"; }
  static bool isReadable() { return true; }
  static bool isWriteable() { return false; }
  static ChimeraTK::AccessModeFlags supportedFlags() {
    return {ChimeraTK::AccessMode::wait_for_new_data, ChimeraTK::AccessMode::raw};
  }
};

/**********************************************************************************************************************/

struct Scalar32Map2 : ScalarDescriptor<Scalar32Map2> {
  static std::string path() { return "FCM.WORD_REV_SWITCH"; }
  static bool isReadable() { return true; }
  static bool isWriteable() { return true; }
};

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testBrokenMap) {
  Device dev;
  dev.open(cdd);
  BOOST_CHECK_THROW(dev.getScalarRegisterAccessor<int32_t>("/BROKEN/REG"),
      ChimeraTK::logic_error); // NOLINT(clang-diagnostic-unused-result)
  dev.close();
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testUnified) {
  UnifiedBackendTest<>()
      .addRegister<Scalar32Map0>()
      .addRegister<Scalar32Map1>()
      .addRegister<Scalar32Map1Async>()
      .addRegister<Scalar32Map2>()
      .runTests(cdd);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
