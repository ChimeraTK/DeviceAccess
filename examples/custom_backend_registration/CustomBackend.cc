// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <ChimeraTK/BackendFactory.h>
#include <ChimeraTK/DeviceAccessVersion.h>
#include <ChimeraTK/DummyBackend.h> // Would probably be DeviceBackendImpl or NumericAddressedBackend in a real application

#include <boost/make_shared.hpp>

/*
 * A custom backend which is registered to the factory.
 * This example only shows how to register a new type of backend to the factory.
 * It does not show how to write a new backend. We are lazy and derrive from
 * DummyBackend to have a fully working backend. In a real example you would
 * either derrive from DeviceBackendImpl or NumericAddressedBackend, unless you
 * want to write a custom dummy for testing.
 *
 * Custom backends are always created as a shared library which can be loaded at
 * run time.
 */
class CustomBackend : public ChimeraTK::DummyBackend {
 public:
  // C++11 shorthand syntax that we want a constructor with the same parameters
  // as the parent class.
  using ChimeraTK::DummyBackend::DummyBackend;

  /*
   * You have to implement a static function createInstance() with this exact
   * signature. This function is later given to the BackendFactory to create
   * this type of backend when it is requested.
   */
  static boost::shared_ptr<ChimeraTK::DeviceBackend> createInstance(
      std::string /*address*/, std::map<std::string, std::string> parameters) {
    /*
     * Inside createInstance the parameters are interpreted and passed on to the
     * constructor. Like this the backend constructor can have arbitrary
     parameters
     * while the factory can always call a function with the same signature.

     * In this example we have to convert the "map" parameter to an absolute
     path
     * (there is already a function for it in the DummyBackend parent class),
     * and pass it on to the constructor, which has the same signature as
     DummyBackend
     * (see 'using' clause above).
     *
     * This part will vary, depending on the requirements of the particular
     backend.
     */
    std::string absolutePath = convertPathRelativeToDmapToAbs(parameters["map"]);

    /*
     * Now we have all parameters for the constructor. We just have to create a
     * shared pointer of the CustomBackend with it.
     */
    return boost::make_shared<CustomBackend>(absolutePath);
  }

  /*
   * The task of the BackendRegister is to call the function which tells the
   * factory about the new type of backend. This is happening in the constructor
   * of the class, so you just have to create an instance of the class and the
   * code is executed.
   */
  struct BackendRegisterer {
    BackendRegisterer() {
      /*
       * The first parameter is the backend type string. It is the name by which
       * the factory knows which type of backend to create. The name has to be
       * unique. It shows up in the ChimeraTK device descriptor, in this case
       * (CUSTOM?map=example.map)
       * (example.map is the parameter which is passed on to createInstance, see
       * above)
       *
       * The second parameter is the pointer to the createInstance function.
       * The factory stores this pointer together with the type name and call
       * the functions when this type if backed needs to be created.
       */
      ChimeraTK::BackendFactory::getInstance().registerBackendType("CUSTOM", &CustomBackend::createInstance);
    }
  };
};

// We have one global instance if the BackendRegisterer. Whenever the library
// containing this backend is loaded, this object is instantiated. As the
// constructor of this class is registering the device, the backend is
// automatically known to the factory when the library is loaded.
static CustomBackend::BackendRegisterer gCustomBackendRegisterer;
