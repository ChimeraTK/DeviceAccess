Technical specification: propagation of initial values {#spec_initialValuePropagation}
======================================================

> **DRAFT VERSION, WRITE-UP IN PROGRESS!**

## Introduction ##

This document describes how initial values are propagated from the control system persistency layer, from the devices and from application modules into the attached components (control system, devices and other application modules).

This specification goes beyond ApplicationCore. It has impact on other ChimeraTK libraries like DeviceAccess, the ControlSystemAdapter and even backends and adapter implementations.

## Definitions ##

- Initial value: The start value of a process variable. The value is available to the receiving end of the process variable at a well defined point in time at the start. This is a logical concept. It is to be distinguished from the (often hardcoded) "value after construction" of the `ChimeraTK::ProcessArray` or any other `ChimeraTK::NDRegisterAccessor` implementation. The point in time when the value becomes available is well-defined, as described in the high-level requirements.

## High-level requirements ##

- Initial values must be available to all `ChimeraTK::ApplicationModule`s at the start of the `ChimeraTK::ApplicationModule::mainLoop()`. No call to `ChimeraTK::TransferElement::read()` etc. is required. This implies that the `ChimeraTK::ApplicationModule::mainLoop()` is not started until all initial values are available, including those coming from devices which might potentially be offline, or from other `ApplicationModule`s.
- `ChimeraTK::ApplicationModule` implementations can either provide initial values for their outputs in `ChimeraTK::ApplicationModule::prepare()` (if the initial value doesn't depend on any inputs) or right after the start of the `ChimeraTK::ApplicationModule::mainLoop()` (if the initial value needs to be computed from the incoming initial values of the inputs).
- The "value after construction" must not be propagated automatically during initial value propagation, not even with the `ChimeraTK::DataValidity::faulty` flag set. It must not be visible to user code in the `ChimeraTK::ApplicationModule::mainLoop()`.
- Since `ChimeraTK::ApplicationModule`s might wait for initial values from other `ChimeraTK::ApplicationModule`s, the modules might end up in a dead lock due to a circular connection. The circular connection is legal, but the dead lock situation needs to be broken by one `ChimeraTK::ApplicationModule` writing its initial value during `ChimeraTK::ApplicationModule::prepare()`.
- Devices must receive the initial values as soon as possible after the device is opened and after the initialisation sequence is executed. There is no guarantee that other registers of the same device are written or read only after the initial values are written. Hence, any critical registers that need to be written before accessing other registers must be written in the initialisation sequence.
- The control system doesn't receive "initial values" as such. The first value of a process variable is sent to the control system when available. This may depend even on external conditions like the availability of devices, e.g. the control system interface has to run even if devices are not available and hence cannot send an inital value.
- Control system variables show the `ChimeraTK::DataValidity::faulty` flag until they have received the first valid value.
- For push-type variables from devices, the initial value is the current value polled at the application start. Since the variable might not get pushed regularly, the application must not wait for a value to get pushed.

## Detailed requirements ##

1. All `ChimeraTK::NDRegisterAccessor` implementations (including but not limited to the `ChimeraTK::ProcessArray`) must have the `ChimeraTK::DataValidity::faulty` flag set after construction for the receiving end. This ensures, all data is marked as `faulty` as long as no sensible initial values have been propagated. The sending end must have `ChimeraTK::DataValidity::ok` after construction, so that the first written data automatically propagates the ok state by default. For bidirectional variables, this must be the case for both directions separately.
2. All `ChimeraTK::NDRegisterAccessor` implementations must have initially a `ChimeraTK::VersionNumber` constructed with a `nullptr`, which allows to check whether this variable is still at its "value after construction", or the initial value propagation already took place.
3. `ChimeraTK::ApplicationModule` (and `ChimeraTK::ThreadedFanOut`/`ChimeraTK::TriggerFanOut`) propagate the `ChimeraTK::DataValidity` of its outputs according to the state of all inputs. This behaviour is identical to later during normal data processing.
4. (removed)
5. Control system variables:
  1. Variables with the control-system-to-application direction must be written exactly once at application start by the control system adapter with their initial values from the persistency layer. This must be done before `ChimeraTK::ApplicationBase::run()` is called, or soon after (major parts of the application will be blocked until it's done). If the persistency layer can persist the `ChimeraTK::DataValidity`, the initial value should have the correct validity. Otherwise, initial values will always have the `ChimeraTK::DataValidity::ok`.
  2. Variables with the application-to-control-system direction do not have an "initial value". The first value of these variables are written at an undefined time after the `ChimeraTK::ApplicationBase::run()` has been called. The control system adapter must not expect any specific behaviour. Entities writing to these variables do not need to take any special precautions.
6. Device variables:
  1. Write accessors need to be written after the device is opened and the initialisation is done, as soon as the initial value is available for that variable. Initial values can be present through 5.a, 6.b or 7.
  2. Initial values for read accessors must be read after the device is openend and the initialsation is done. The read is performed with `ChimeraTK::TransferElement::readLatest()`.
7. Outputs of `ChimeraTK::ApplicationModule`s:
  1. Initial values can be written in `ChimeraTK::ApplicationModule::prepare()`, if the value does not depend on any input values (since input values are not available during `prepare()`).
  2. Alternatively, initial values can be written in `ChimeraTK::ApplicationModule::mainLoop()` before calling any `read` function. Typically, to propagate the initial values of its inputs, an `ApplicationModule` will run its computations and write its outputs first before waiting for new data with a blocking `read()` and the end of the processing loop. The application author needs to take into account that in this case `write` functions might block until the target device becomes available and hence block the further propagation of the initial values.
  3. Since in `ChimeraTK::ApplicationModule::prepare()` all devices are still closed, any writes to device variables at this point need to be delayed until the device is open. The actual write is hence performed asynchronously in a different thread.
8. Inputs of `ChimeraTK::ApplicationModule`s:
  1. Initial values are read before the start of `ChimeraTK::ApplicationModule::mainLoop()` (but already in the same thread which later executes the `mainLoop()`).
  2. It depends on the data source type (i.e. the type of the feeder of the VariableNetwork) whether a blocking `read()` or non-blocking `readLatest()` needs to be used for the initial value:
    1. *control system variable*: `ChimeraTK::TransferElement::read()`
    2. *device register without trigger*: `ChimeraTK::TransferElement::readLatest()` (even if register is push-type). Special treatment required to block until the device is accessible.
    3. *poll-type device register with trigger (incl. `ChimeraTK::VariableNetwork::TriggerType::pollingConsumer`)*: `ChimeraTK::TransferElement::read()`
    4. *constant*: `ChimeraTK::TransferElement::readLatest()`
    5. *application*: `ChimeraTK::TransferElement::read()`
9. The module-like fan outs `ChimeraTK::ThreadedFanOut` and `ChimeraTK::TriggerFanOut` (does not apply to the accessor-like fan outs `ChimeraTK::FeedingFanOut` and `ChimeraTK::ConsumingFanOut`)
  1. The fan outs should have a transparent behaviour, i.e. an entity that receives an initial value through a fan out should see the same behaviour as if a direct connection would have been realised.
  2. This implies that the inputs need to treated like described in 8.b.
  3. The initial value is propagated immediately to the outputs.
  4. If an output cannot be written at that point (because it writes to a device currently being unavailable), the value propagation to other targets must not be blocked. See recovery mechanism described in @ref exceptionHandlingDesign.
10. Constants (`ChimeraTK::Application::makeConstant()`):
  1. Values are propagated before the `ChimeraTK::ApplicationModule` threads are starting (just like initial values written in `ChimeraTK::ApplicationModule::prepare()`).
  2. Special treatment for constants written to devices: They need to be written after the device is opened (see 6.a), with the same mechanism as in 7.c.
11. Variables with a return channel ("bidirectional variables", `ChimeraTK::ScalarPushInputWB`, `ChimeraTK::ScalarOutputPushRB` and the array variants) behave like their unidirectional pendants, i.e. the existence of the return channel is ignored during the initial value propagation.

### Comments ###

- To 4.: It is very important that no wrong data is transported initially. Since the "value after construction" of all process variables is always 0 or so, this value is basically always wrong. If it gets propagated within the application, modules will process this value (usually even if `ChimeraTK::DataValidity::faulty` is set), despite the value might present an inconsistent state with other process variables. If it gets propagated to the control system, other applications might act on an again inconsistent state.
- To 5.: This is the responsibility of each control system adpater implementation.
- To 7. and 10.: An output of a `ChimeraTK::ApplicationModule` with an initial value written in `ChimeraTK::ApplicationModule::prepare()` and later never written again behaves in the same way as a constant.
- To 7.b.: In future, the specification could be changed to mitigate the issue of blocking `write`s: It could be specified that all `write`s in the `ChimeraTK::ApplicationModule::mainLoop()` are not blocking due to unavailable devices, until the first `read` function has been called.
- To 8.b.: The decision whether to use blocking or non-blocking read for the initial transfer has the following reasons:
  - 8.b.i.: Blocking reads prevent race condtion especially in cases where a ThreadedFanOut is involved.
  - 8.b.ii.: `ChimeraTK::TransferElement::readLatest()` fetches current value instead of waiting for a new value - see high-level requirements.
  - 8.b.iii.: Blocking reads prevent race condtion.
  - 8.b.iv.: Blocking reads on constants never return, hence the non-blocking read.
  - 8.b.v.: Blocking read required in case the sender writes the initial value during `ChimeraTK::ApplicationModule::mainLoop()`.


## Implementation ##

### NDRegisterAccessor implementations ###

- Each `ChimeraTK::NDRegisterAccessor` must implement 1. separately.
- Each `ChimeraTK::NDRegisterAccessor` must implement 2. separately. All accessors should already have a `ChimeraTK::VersionNumber` data member called `currentVersion` or similar, it simply needs to be constructed with a `nullptr` as an argument.

### ApplicationModule ###

- Implement 3.:
  - `ChimeraTK::ApplicationModule::getDataValidity()` returns `ok` if the `faultCounter` is 0, otherwise `faulty`.
  - All input variables are `faulty` at start.
  - Hence fault counter should start at the number of inputs.
  - The `ChimeraTK::MetaDataPropagatingDecorator` will count down the fault counter when an `ok` value is received
  - Hence `ChimeraTK::ApplicationModule::getDataValidity()` will return `ok` when all inputs have received an `ok` value.
- API documentation must contain 7.
- Implements 8. (hence takes part in 5.a, 6.b, 7 and 10 implicitly):
  - All inputs of the module must be read in the `ChimeraTK::ApplicationModule::mainLoopWrapper()` before the call to `mainLoop()`.
  - The type of the read decided via `ChimeraTK::VariableNetworkNode::hasInitialValue()`, which implements 8.b. [tbd: new name]

### ThreadedFanOut ###

- Implement 3, implementation will be already covered by normal flag propagation
- Needs to implement 9. (hence takes part in 5.a, 6, 7 and 10 implicitly):
  - structure the fan out's "mainLoop" like this:
    - read initial values (9.a via `ChimeraTK::VariableNetworkNode::hasInitialValue()` [tbd: new name])
    - begin loop
    - write outputs
    - read input
    - cycle loop
  - 9.d is covered by the `ChimeraTK::ExceptionHandlingDecorator`.

### TriggerFanOut ###

- Needs to implement 3, implementation will be already covered by normal flag propagation
- Needs to implement 9. (hence takes part in 5.a, 6, 7 and 10 implicitly):
  - structure the fan out's "mainLoop" like this:
    - read initial values (9.a via `ChimeraTK::VariableNetworkNode::hasInitialValue()` [tbd: new name])
    - begin loop
    - write outputs
    - read input
    - cycle loop
  - 9.d **not** covered by the `ChimeraTK::ExceptionHandlingDecorator` here (as the decorator is not effective with `ChimeraTK::TransferGroup`s), needs to be properly implemented [tbd: if all DeviceAccess backends are obliged to throw exceptions *only* in `postRead()`, the decorator could be effective also for `TransferGroup`s and we would also solve some issues with exceptions during asynchronous transfers and `readAny()`]

### DeviceModule ###

- All points are also covered by @ref exceptionHandlingDesign.
- Takes part in 6.a:
  - `ChimeraTK::DeviceModule::writeAfterOpen/writeRecoveryOpen` [tbd: new name for unified list] is a list of accessors to be written after the device is opened/recovered.
- Takes part in 6.b:
  - Initial values are being read from the device by other entities, but these must be blocked until the DeviceModule wakes them up after the device has been opened and initialised. This uses the same mechanism as for blocking read operations during recovery.
- Needs to implement 10.b, done through `ChimeraTK::DeviceModule::writeAfterOpen/writeRecoveryOpen` -> already covered by first point.

### ExceptionHandlingDecorator ###

- Must implement part of 6.a/7.c/9.d/10.b: Provide function which allows to write without blocking in case of an unavailable device:
  - The list `ChimeraTK::DeviceModule::writeAfterOpen/writeRecoveryOpen` [tbd: new name for unified list] is filled with the "recovery accessor" (a "copy" of the original accessor, created by `ChimeraTK::Application::createDeviceVariable()`). This accessor allows the restoration of the last known value of a register after recovery from an exception by the DeviceModule. See also @ref exceptionHandlingDesign.
  - When a write happens while the device is still unavailable (not opened or initialisation still in progress), the write should not block (in contrast to normal writes in a `ChimeraTK::ApplicationModule::mainLoop()`).
  - The "recovery accessor" is also used in this case to defer the write operation until the device becomes available.
  - The actual write is then performed asynchronously by the `ChimeraTK::DeviceModule`.
  - This implementation is the same as the implementation for the non-blocking write function [tbd: correct name] which available to `ChimeraTK::ApplicationModule` user implementations.

- Needs to implement 6.b.:
  - The `ChimeraTK::TransferElement::readLatest()` must be delayed until the device is available and initialised.
  - @ref exceptionHandlingDesign states that non-blocking read operations like `ChimeraTK::TransferElement::readLatest()` should never block due to an exception.
  - Hence a special treatment is required in this case:
  - `ChimeraTK::ExceptionHandlingDecorator::readLatest()` should block until the device is opened and initialised if (and only if) the accessor still has the `ChimeraTK::VersionNumber(nullptr)` - which means it has not yet been read.
  - Since `readLatest()` is always the first `read`-type function called, it is acceptable if all `read`-type functions implement this behaviour.

### VariableNetworkNode ###

- Implements the decision tree mentioned in 8.b. in `ChimeraTK::VariableNetworkNode::hasInitialValue()` [tbd: new name]

### Application ###
(This section refers to the class `ChimeraTK::Application`, not to the user-defined application.)

- Implements 10.a. 10.b covered by `ChimeraTK::ExceptionHandlingDecorator`.

### ControlSystemAdapter ###

- Must implement 5.a
  - Needs to be done in all adapters separately


## Known bugs ##

### DeviceAccess interface ###

- 1. is currently not implementable for (potentially) bidirectional accessors (like most backend accessors). An interface change is required to allow separete `ChimeraTK::DataValidity` flags for each direction.

### NDRegisterAccessor implementations ###

- 1. is not implemented for Device implementations (only the `UnidirectionalProcessArray` is correct at the moment).
- 2. is not implemented for Device implementations (only the `UnidirectionalProcessArray` is correct at the moment).

### TriggerFanOut ###

- 3. is not correctly implemented, it needs to be done on a per-variable level.

### ExceptionHandlingDecorator ###

- It waits until the device is opened, but not until after the initialisation is done.

### ControlSystemAdapter ###

- EPICS-Adapter might not properly implement 5.a, needs to be checked. Especially it might not be guaranteed that all variables are written (and not only those registered in the autosave plugin).
- The `ChimeraTK::UnidirectionalProcessArray` uses always a default start value of `DataValidity::ok`, but overwrites this with `DataValidity::faulty` for the receivers in the factory function `ChimeraTK::UnidirectionalProcessArray::createSynchronizedProcessArray()` (both implementations, see UnidirectionalProcessArray.h). This can be solved more elegant after the DeviceAccess interface change described above.


### Documentation ###

- Documentation of ControlSystemAdapter should mention that implementations must take care about 5.
- Documentation for ApplicationModule should mention 7.