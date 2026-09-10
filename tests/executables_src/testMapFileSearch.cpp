// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE MapFileSearch
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "BackendFactory.h"
#include "DummyBackend.h"
#include "Exception.h"
#include "ProcessManagement.h"
#include "Utilities.h"

#include <boost/filesystem.hpp>

#include <filesystem>
#include <fstream>

using namespace ChimeraTK;

namespace fs = std::filesystem;

/**********************************************************************************************************************/

/// Test-only backend exposing the resolved map file name computed by the
/// NumericAddressedBackend base class. This is a grey-box check of the unified
/// map file search; the attribute is part of the NumericAddressedBackend API.
struct MapFileSearchBackend : public DummyBackend {
  using DummyBackend::DummyBackend;
  std::string resolvedMapFileName() const { return _resolvedMapFileName; }
};

/**********************************************************************************************************************/

struct MapFileSearchFixture {
  fs::path baseDir;
  fs::path workDir;
  fs::path previousWorkDir;

  MapFileSearchFixture() {
    previousWorkDir = fs::current_path();
    baseDir = fs::temp_directory_path() / "testMapFileSearch";
    fs::remove_all(baseDir);
    workDir = baseDir / "work";
    fs::create_directories(workDir);
    // the working directory represents the "current working directory" that
    // relative map paths are resolved against
    fs::current_path(workDir);
    BackendFactory::getInstance().setDMapFilePath("");
  }

  ~MapFileSearchFixture() {
    fs::current_path(previousWorkDir);
    fs::remove_all(baseDir);
  }

  /// Write a map file containing a single register with the given name.
  static void writeMapFile(const fs::path& path, const std::string& registerName) {
    if(!path.parent_path().empty()) {
      fs::create_directories(path.parent_path());
    }
    std::ofstream file(path);
    file << "/Board/" << registerName << " 1 0 4 1 32 0 1 RW\n";
  }

  /// Write a minimal, parseable dmap file referencing the given map file.
  static void writeDMapFile(const fs::path& path, const std::string& mapName) {
    if(!path.parent_path().empty()) {
      fs::create_directories(path.parent_path());
    }
    std::ofstream file(path);
    file << "DUMMY (dummy?map=" << mapName << ")\n";
  }

  static void setDMapPath(const std::string& dmapPath) { BackendFactory::getInstance().setDMapFilePath(dmapPath); }

  static std::string canonical(const std::string& path) { return boost::filesystem::canonical(path).string(); }
};

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE(MapFileSearchTestSuite)

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(RelativeMapResolvedRelativeToDmapDirectory, MapFileSearchFixture) {
  // map file placed next to the dmap file, not present in the cwd
  writeDMapFile("dmapdir/test.dmap", "map1.map");
  writeMapFile("dmapdir/map1.map", "fromDmapDir");
  setDMapPath("dmapdir/test.dmap");

  MapFileSearchBackend backend("map1.map");

  BOOST_CHECK_EQUAL(backend.resolvedMapFileName(), canonical("dmapdir/map1.map"));
  BOOST_CHECK(backend.getRegisterCatalogue().hasRegister("/Board/fromDmapDir"));
}

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(RelativeMapResolvedRelativeToCwd, MapFileSearchFixture) {
  // map file only present in the cwd, dmap file in a different directory
  writeDMapFile("dmapdir/test.dmap", "map2.map");
  writeMapFile("map2.map", "fromCwd");
  setDMapPath("dmapdir/test.dmap");

  MapFileSearchBackend backend("map2.map");

  BOOST_CHECK_EQUAL(backend.resolvedMapFileName(), canonical("map2.map"));
  BOOST_CHECK(backend.getRegisterCatalogue().hasRegister("/Board/fromCwd"));
}

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(MapFileInBothDmapDirAndCwdDmapDirWins, MapFileSearchFixture) {
  // the same map file name in both the dmap directory and the cwd
  writeDMapFile("dmapdir/test.dmap", "map3.map");
  writeMapFile("dmapdir/map3.map", "fromDmapDir");
  writeMapFile("map3.map", "fromCwd");
  setDMapPath("dmapdir/test.dmap");

  MapFileSearchBackend backend("map3.map");

  BOOST_CHECK_EQUAL(backend.resolvedMapFileName(), canonical("dmapdir/map3.map"));
  BOOST_CHECK(backend.getRegisterCatalogue().hasRegister("/Board/fromDmapDir"));
  BOOST_CHECK(!backend.getRegisterCatalogue().hasRegister("/Board/fromCwd"));
}

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(AbsoluteMapPathUsedAsIs, MapFileSearchFixture) {
  // an absolute map path is used directly, independent of the dmap path
  writeDMapFile("dmapdir/test.dmap", "map4.map");
  writeMapFile("other/map4.map", "fromAbsolute");
  setDMapPath("dmapdir/test.dmap");

  std::string absolutePath = canonical("other/map4.map");
  MapFileSearchBackend backend(absolutePath);

  BOOST_CHECK_EQUAL(backend.resolvedMapFileName(), absolutePath);
  BOOST_CHECK(backend.getRegisterCatalogue().hasRegister("/Board/fromAbsolute"));
}

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(NoDmapPathSetRelativeMapResolvedRelativeToCwd, MapFileSearchFixture) {
  // no dmap path set at all: a relative map path resolves relative to the cwd
  writeMapFile("map5.map", "fromCwd");

  MapFileSearchBackend backend("map5.map");

  BOOST_CHECK_EQUAL(backend.resolvedMapFileName(), canonical("map5.map"));
  BOOST_CHECK(backend.getRegisterCatalogue().hasRegister("/Board/fromCwd"));
}

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(RelativeDmapPathDmapDirRelativeToCwd, MapFileSearchFixture) {
  // a relative dmap path is interpreted relative to the cwd
  writeDMapFile("dmapdir/sub/test.dmap", "map6.map");
  writeMapFile("dmapdir/sub/map6.map", "fromDmapDirSub");
  setDMapPath("dmapdir/sub/test.dmap");

  MapFileSearchBackend backend("map6.map");

  BOOST_CHECK_EQUAL(backend.resolvedMapFileName(), canonical("dmapdir/sub/map6.map"));
  BOOST_CHECK(backend.getRegisterCatalogue().hasRegister("/Board/fromDmapDirSub"));
}

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(MissingMapFileThrowsClearError, MapFileSearchFixture) {
  // a missing map file leads to a clear error, not a crash
  writeDMapFile("dmapdir/test.dmap", "nonexistent.map");
  setDMapPath("dmapdir/test.dmap");

  BOOST_CHECK_THROW(MapFileSearchBackend backend("nonexistent.map"), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(SharedDummyShmNameDerivedFromResolvedPath, MapFileSearchFixture) {
  // two different relative spellings of the same map file resolve to the same
  // canonical absolute path, so the shared dummy SHM segment name (derived from
  // the resolved path) is identical for both: no asymmetry between the map
  // parsing and the SHM hash
  writeMapFile("shared.map", "fromCwd");

  MapFileSearchBackend backend1("shared.map");
  MapFileSearchBackend backend2("./shared.map");

  BOOST_CHECK_EQUAL(backend1.resolvedMapFileName(), canonical("shared.map"));
  BOOST_CHECK_EQUAL(backend1.resolvedMapFileName(), backend2.resolvedMapFileName());

  std::size_t instanceIdHash = Utilities::shmDummyInstanceIdHash("1", {{"map", "shared.map"}});
  std::string userName = getUserName();
  BOOST_CHECK_EQUAL(Utilities::createShmName(instanceIdHash, backend1.resolvedMapFileName(), userName),
      Utilities::createShmName(instanceIdHash, backend2.resolvedMapFileName(), userName));
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
