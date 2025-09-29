# ChimeraTK DeviceAccess

The ChimeraTK DeviceAccess library provides an abstract interface for register based devices. Registers are identified by a name and usually accessed though an accessor object. Since this library also allows access to other control system applications, it can be understood as the client library of the ChimeraTK framework.

The API documentation and tutorial examples for all versions are available under<br>
<a href="https://chimeratk.github.io/DeviceAccess/index.html" target="_blank">https://chimeratk.github.io/DeviceAccess/index.html</a>

## Dependencies

To build this library from source, following libraries have to be built / installed first (for details check CMakeLists.txt and check `FIND_PACKAGE` statements):

* `Boost` components `thread`, `system`, `chrono` and `filesystem` (e.g. Ubuntu/Debian: `apt install libboost-thread-dev libboost-system-dev libboost-chrono-dev libboost-filesystem-dev`)
* [nlohmann_json](https://github.com/nlohmann/json)
* [cppext](https://github.com/ChimeraTK/cppext)
* [exprtk](https://github.com/ChimeraTK/exprtk) via [exprtk-interface](https://github.com/ChimeraTK/exprtk-interface) (use `git clone --recurse-submodules`)
