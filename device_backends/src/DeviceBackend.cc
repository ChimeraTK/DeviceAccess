/*
 * DeviceBackend.cc
 */

#include <tuple>

#include "DeviceBackend.h"
#include "BackendBufferingRegisterAccessor.h"
#include "FixedPointConverter.h"

namespace mtca4u {

  DeviceBackend::~DeviceBackend() {
  }

  /********************************************************************************************************************/

  namespace DeviceBackendHelper {

    /// Helper class to implement the templated getBufferingRegisterAccessor function with boost::fusion::for_each.
    /// This allows to avoid writing an if-statement for each supported user type.
    class getBufferingRegisterAccessorImplClass
    {
      public:
        getBufferingRegisterAccessorImplClass(const std::type_info &_type, const std::string &_registerName,
            const std::string &_module, boost::shared_ptr<DeviceBackend> _backend)
        : type(_type), registerName(_registerName), module(_module), backend(_backend), ptr(NULL)
        {}

        /// This function will be called by for_each() for each type in FixedPointConverter::userTypeMap
        template <typename Pair>
        void operator()(Pair) const
        {
          // obtain UserType from given fusion::pair type
          typedef typename Pair::first_type ConvertedDataType;

          // check if UserType is the requested type
          if(typeid(ConvertedDataType) == type) {
            BackendBufferingRegisterAccessor<ConvertedDataType> *typedptr;
            typedptr = new BackendBufferingRegisterAccessor<ConvertedDataType>(backend,registerName,module);
            ptr = static_cast<void*>(typedptr);
          }
        }

        /// Arguments passed from getBufferingRegisterAccessorImpl
        const std::type_info &type;
        const std::string &registerName;
        const std::string &module;
        boost::shared_ptr<DeviceBackend> backend;

        /// pointer to the created accessor (or NULL if an invalid type was passed)
        mutable void *ptr;
    };

  }

  /********************************************************************************************************************/

  void* DeviceBackend::getBufferingRegisterAccessorImpl(const std::type_info &UserType, const std::string &registerName,
      const std::string &module) {
    FixedPointConverter::userTypeMap userTypes;
    DeviceBackendHelper::getBufferingRegisterAccessorImplClass impl(UserType, registerName, module, shared_from_this());
    boost::fusion::for_each(userTypes, impl);
    if(!impl.ptr) {
      throw DeviceException("Unknown user type passed to DeviceBackend::getBufferingRegisterAccessorImpl()",
          DeviceException::EX_WRONG_PARAMETER);
    }
    return impl.ptr;
  }

}
