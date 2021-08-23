#include "NumericAddressedInterruptDispatcher.h"

namespace ChimeraTK {

  NumericAddressedInterruptDispatcher::NumericAddressedInterruptDispatcher() {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(createAsyncVariable);
  }

  //*********************************************************************************************************************/
  VersionNumber NumericAddressedInterruptDispatcher::trigger() {
    VersionNumber ver; // a common VersionNumber for this trigger. Must be generated under mutex

    executeForEach(
        [](std::unique_ptr<AsyncVariable>& asyncVariable, VersionNumber const& v) {
          auto numericAddressAsyncVariable = dynamic_cast<NumericAddressedAsyncVariable*>(asyncVariable.get());
          assert(numericAddressAsyncVariable);
          numericAddressAsyncVariable->trigger(v);
        },
        ver);

    return ver;
  }

  INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(AsyncNDRegisterAccessor, NumericAddressedInterruptDispatcher,
      NumericAddressedInterruptDispatcher::AccessorInstanceDescriptor);

} // namespace ChimeraTK
