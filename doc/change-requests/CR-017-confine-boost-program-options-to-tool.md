# CR-017: Confine Boost program_options to the device-to-jmap tool

Synopsis: Stop the exported ChimeraTK-DeviceAccess target from requiring Boost
program_options, which the library does not use and which is needed only by the
`chimeratk-device-to-jmap` executable.

Status: DONE

## Requirements

Exported dependency set:

- Downstream projects using `find_package(ChimeraTK-DeviceAccess)` must not need
Boost program_options.
- The installed `ChimeraTK-DeviceAccessTargets.cmake` must not reference
`Boost::program_options` in its interface.
- The installed `ChimeraTK-DeviceAccessConfig.cmake` must not request the
program_options component.
- The library keeps its other public Boost components: thread, system, chrono,
filesystem.
- `chimeratk-device-to-jmap` keeps using Boost program_options.

## Specifications

Affected component: top-level `CMakeLists.txt` only.

Library target:

- Replace `FIND_PACKAGE(Boost COMPONENTS thread system chrono filesystem
program_options REQUIRED)` with the four components only.
- Replace `PUBLIC ${Boost_LIBRARIES}` with the explicit imported targets
`Boost::thread`, `Boost::system`, `Boost::chrono`, `Boost::filesystem`.
- `${PROJECT_NAME}_PUBLIC_DEPENDENCIES` already lists `system thread chrono
filesystem`, so it stays unchanged.

Tool target:

- Add `FIND_PACKAGE(Boost COMPONENTS program_options REQUIRED)` in the tool
section, next to `chimeratk-device-to-jmap`.
- Link `Boost::program_options` PRIVATE to `chimeratk-device-to-jmap` and drop
the `${Boost_LIBRARIES}` use there.

Design choices:

- Component imported targets are used instead of the `Boost_LIBRARIES` variable
so the exported interface names exactly the required components instead of the
whole shared variable.
- Naming the components explicitly makes the exported dependency set
independent of the shared `Boost_LIBRARIES` variable.
- `tests/CMakeLists.txt` is out of scope: its `${Boost_LIBRARIES}` uses are
PRIVATE on test executables and cannot affect the exported library interface,
which is recorded before the tests subdirectory is added.

### Alternatives considered

- Keep the variable, only drop program_options from the library `FIND_PACKAGE`
and add a second `FIND_PACKAGE` for the tool. The variable-based variant would
also remove program_options from the interface, since the interface is
recorded before the tests are added. Rejected: it relies on the shared
`Boost_LIBRARIES` variable instead of naming the required components
explicitly.

## Test plan

- Executed once, manually by the agent; no automated unit test is added for this
packaging change.
- Install the project and assert `ChimeraTK-DeviceAccessTargets.cmake` lists the
four Boost targets and not `program_options`.
- Assert `ChimeraTK-DeviceAccessConfig.cmake` requests system, thread, chrono and
filesystem, not program_options.
- Configure and build a minimal consumer through
`find_package(ChimeraTK-DeviceAccess)` linking
`ChimeraTK::ChimeraTK-DeviceAccess`.
- Build and run `chimeratk-device-to-jmap`.
