Technical specification: propagation of initial values {#spec_initialValuePropagation}
======================================================

> **DRAFT VERSION, WRITE-UP IN PROGRESS!**

## Introduction ##

This document describes how initial values are propagated from the control system persistency layer, from the devices and (if applicable) from application modules into the attached components (control system, devices and other application modules).

This specification goes beyond ApplicationCore. It has impact on other ChimeraTK libraries like DeviceAccess, the ControlSystemAdapter and even backends and adapter implementations.

## Definitions ##

- Initial value: The start value of a process variable. The value is available to the receiving end of the process variable at a well defined point in time at the start. This is a logical concept. It is to be distinguished from the (often hardcoded) "value after construction" of the `ChimeraTK::ProcessArray` or any other `ChimeraTK::NDRegisterAccessor` implementation. The point in time when the value becomes available is well-defined, as described in the high-level requirements.

## High-level requirements ##

- Initial values must be available to all `ChimeraTK::ApplicationModule`s at the start of the `ChimeraTK::ApplicationModule::mainLoop()`. No call to `ChimeraTK::TransferElement::read()` etc. is required. This implies that the `ChimeraTK::ApplicationModule::mainLoop()` is not started until all initial values are available, including those coming from devices which might potentially be offline.
- Devices must receive the initial values as soon as possible after the device is opened and after the initialisation sequence is executed. There is no guarantee that other registers of the same device are written or read only after the initial values are written. Hence, any critical registers that need to be written before accessing other registers must be written in the initialisation sequence.
- The control system doesn't receive "initial values" as such. The first value of a process variable is sent to the control system when available. This may depend even on external conditions like the availability of devices.
- Control system variables show the `ChimeraTK::DataValidity::faulty` flag until they have received the first valid value.
- For push-type variables from devices, the initial value is the current value polled at the application start. Since the variable might not get pushed regularly, the application must not wait for a value to get pushed.

## Detailed requirements ##

1. All `ChimeraTK::NDRegisterAccessor` implementations (including but not limited to the `ChimeraTK::ProcessArray`) must have the `ChimeraTK::DataValidity::faulty` flag set after construction for the receiving end. This ensures, all data is marked as `faulty` as long as no sensible initial values have been propagated. The sending end must have `ChimeraTK::DataValidity::ok` after construction, so that the first written data automatically propagates the ok state. For bidirectional variables, this must be the case for both directions separately.
2. All `ChimeraTK::NDRegisterAccessor` implementations must have initially a `ChimeraTK::VersionNumber` constructed with a `nullptr`, which allows to check whether this variable is still at its "value after construction", or the initial value propagation already took place.
3. `ChimeraTK::ApplicationModule` (and `ChimeraTK::ThreadedFanOut`/`ChimeraTK::TriggerFanOut`) need to set the `ChimeraTK::DataValidity` of its outputs according to the state of all inputs after the initial value propagation: If any input has received a `ChimeraTK::DataValidity::faulty` flag or has not recieved an initial value at all, the initial `ChimeraTK::DataValidity` of all outputs will be set to `faulty` when written. Otherwise the `ChimeraTK::DataValidity` is `ok`.
  1. `ApplicationModule` implementations might want to check whether all inputs are `ok` before starting the data processing.
  2. If 3.a. is not implemented, it is possible that the module starts processing with data containing or based on "values after construction". This is particularly the case if the module processes the inital values first before waiting on new incoming data. The output values in this case will be marked as `ChimeraTK::DataValidity::faulty`. Which behaviour is wanted needs to be decided for each module implementation by the application author.
4. The "value after construction" must not be propagated automatically during initial value propagation, not even with the `ChimeraTK::DataValidity::faulty` flag set.
5. Control system variables:
  1. Variables with the control-system-to-application direction must be written exactly once at application start by the control system adapter with their initial values from the persistency layer. This must be done before `ChimeraTK::ApplicationBase::run()` is called. If the persistency layer can persist the `ChimeraTK::DataValidity`, the initial value should have the correct validity. Otherwise, initial values will always have the `ChimeraTK::DataValidity::ok`.
  2. Variables with the application-to-control-system direction do not have an "initial value". The first value of these variables are written at an undefined time after the `ChimeraTK::ApplicationBase::run()` has been called. The control system adapter must not expect any specific behaviour. Entities writing to these variables do not need to take any special precautions.
6. Device variables:
  1. Write accessors need to be written right after the device is opened and the initialisation is done, if an initial value is present for that variable. Initial values can be present through 5.a or 7.
  2. Initial values for read accessors must be read after the device is openend and the initialsation is done. The read is performed with `ChimeraTK::TransferElement::readLatest()`.
7. Outputs of `ChimeraTK::ApplicationModule`s:
  1. By default, no initial values are propagated.
  2. Initial values can be written in `ApplicationModule::prepare()`. This fact is recorded in the variable model (`ChimeraTK::VariableNetworkNode`), see 8.b.v
  3. Since in `ChimeraTK::ApplicationModule::prepare()` all devices are still closed, any writes to device variables at this point need to be delayed until the device is open. The actual write is hence performed by the DeviceModule.
  4. An output of a `ChimeraTK::ApplicationModule` with an initial value written in `ChimeraTK::ApplicationModule::prepare()` and later never written again behaves in the same way as a constant (see 10.).
8. Inputs of `ChimeraTK::ApplicationModule`s:
  1. Initial values are read before the start of `ChimeraTK::ApplicationModule::mainLoop()` (but already in the same thread which later executes the `mainLoop()`).
  2. Since not all variables have initial values (see 7.a), the variable model (`ChimeraTK::VariableNetworkNode`) needs to be checked whether an initial value is present and how it needs to be read. This dependes on the data source type (i.e. the type of the feeder of the VariableNetwork):
    1. *control system variable*: `ChimeraTK::TransferElement::read()`
    2. *device register without trigger*: `ChimeraTK::TransferElement::readLatest()` (even if register is push-type)
    3. *poll-type device register with trigger (incl. `ChimeraTK::VariableNetwork::TriggerType::pollingConsumer`)*: `ChimeraTK::TransferElement::read()`
    4. *constant*: `ChimeraTK::TransferElement::readLatest()`
    5. *application*: `ChimeraTK::TransferElement::read()` only if initial value was provided (see 7.), otherwise no read
9. `ChimeraTK::ThreadedFanOut` and `ChimeraTK::TriggerFanOut` etc. (does not apply to decorator-like fan outs `ChimeraTK::FeedingFanOut` and `ChimeraTK::ConsumingFanOut` etc.)
  1. The FanOuts should have a transparent behaviour, i.e. an entity that receives an initial value through a FanOut should see the same behaviour as if a direct connection would have been realised.
  2. This implies that the inputs need to treated like described in 8.b.
  3. If an input has an initial value, it will be propagated immediately to the outputs.
  4. If an output cannot be written at that point (because it writes to a device currently being unavailable), the value propagation to other targets must not be blocked. The write instead must be delayed using the same mechanism as in 7.c.
10. Constants:
  1. Values are propagated before the `ChimeraTK::ApplicationModule` threads are starting (just like initial values written in `ChimeraTK::ApplicationModule::prepare()`, see 7.d).
  2. Special treatment for constants written to devices: They need to be written after the device is opened (see 6.a), with the same mechanism as in 7.c.
11. Variables with a return channel ("bidirectional variables", `ChimeraTK::ScalarPushInputWB`, `ChimeraTK::ScalarOutputPushRB` and the array variants) behave like their unidirectional pendants, i.e. the existence of the return channel is ignored during the initial value propagation.

### Comments ###

- To 3.a/b.: A seemingly useful alternative were to delay the processing loop until all inputs have a `ChimeraTK::VersionNumber` not equal to `ChimeraTK::VersionNumber(nullptr)`, to ensure all inputs have received values. This might not work as intended, since the input values might be received from other modules which pass on the initial values unconditionally. Hence you still might process data indirectly based on "values after construction" instead of meaningful data.
- To 4.: It is very important that no wrong data is transported initially. Since the "value after construction" of all process variables is always 0 or so, this value is basically always wrong. If it gets propagated within the application, modules will process this value (usually even if `ChimeraTK::DataValidity::faulty` is set), despite the value might present an inconsistent state with other process variables. If it gets propagated to the control system, other applications might act on an again inconsistent state.
- To 5.: This is the responsibility of each control system adpater implementation.
- To 8.b: The decision whether to use blocking or non-blocking read for the initial transfer has the following reasons:
  - 8.b.i.: Blocking reads prevent race condtion especially in cases where a ThreadedFanOut is involved.
  - 8.b.ii.: `ChimeraTK::TransferElement::readLatest()` fetches current value instead of waiting for a new value - see high-level requirements.
  - 8.b.iii.: Blocking reads prevent race condtion.
  - 8.b.iv.: Blocking reads on constants never return, hence the non-blocking read.
  - 8.b.v.: Type of read actually does not matter, since there cannot be a threaded FanOut between application modules  and the source writes in `ApplicationModule::prepare()`.

## Implementation ##

### NDRegisterAccessor implementations ###

- 1. must currently be implemented by each `ChimeraTK::NDRegisterAccessor` separately. [TBD: Instead of requiring all implementations to be changed, we could also fix this in `ChimeraTK::Application::createDeviceVariable()`, but this creates an asymetry to the `ProcessArray`...]
- 2. must currently be implemented by each `ChimeraTK::NDRegisterAccessor` separately. All accessors should already have a `ChimeraTK::VersionNumber` data member called `currentVersion` or similar, it simply needs to be constructed with a `nullptr` as an argument.
- The `ChimeraTK::UnidirectionalProcessArray` uses always a default start value of `DataValidity::ok`, but overwrites this with `DataValidity::faulty` for the receivers in the factory function `ChimeraTK::UnidirectionalProcessArray::createSynchronizedProcessArray()` (both implementations, see UnidirectionalProcessArray.h).

### ApplicationModule ###

- Needs to implement 3:
  - `ChimeraTK::ApplicationModule::getDataValidity()` returns `ok` if the `faultCounter` is 0, `faulty` otherwise
  - Hence fault counter starts with 0.
- API documentation must contain 7
- Needs to implement 8 via `ChimeraTK::VariableNetworkNode::hasInitialValue()`

### ThreadedFanOut ###

- Needs to implement 3.
  - Currently just passing on the validity from the input.
  - This is probably going to change when the correct propagation of the validity flag is implemented.
- Needs to implement 6.a abd 6.b, realised via `ChimeraTK::VariableNetworkNode::hasInitialValue()`
- Needs to implement 9:
  - 9.a via `ChimeraTK::VariableNetworkNode::hasInitialValue()` -> already covered with the previous point
  - 9.b covered by the `ChimeraTK::ExceptionHandlingDecorator`
  - 9.c requires no special precautions

### TriggerFanOut ###

- Needs to implement 3.
- Needs to implement 6.a and 6.b, realised via `ChimeraTK::VariableNetworkNode::hasInitialValue()`
- Needs to implement 9:
  - 9.a via `ChimeraTK::VariableNetworkNode::hasInitialValue()` -> already covered with the previous point
  - 9.b **not** covered by the `ChimeraTK::ExceptionHandlingDecorator` here (as the decorator is not effective with `ChimeraTK::TransferGroup`s), needs to be properly implemented
  - 9.c requires no special precautions

### DeviceModule ###

- Needs to implement parts of 6.a:
  - `ChimeraTK::DeviceModule::writeAfterOpen` is a list of accessors to be written after the device is opened for the first time
  - The `ChimeraTK::ExceptionHandlingDecorator` will fill the list when necessary, see 7.c
- Needs to take care of 6.b partially:
  - Initial values are being read from the device by other entities, but these must be blocked until the DeviceModule wakes them up.
  - This wake up must take place after initialisation and 6.a is complete
- Needs to implement 10.b, done through `ChimeraTK::DeviceModule::writeAfterOpen` -> already covered by previous point.

### ExceptionHandlingDecorator ###

- Needs to implement parts of 6.a:
  - When a write happens while the application's `LifeCycleState` is not yet `run`, it must not block, because the device will not be openend, see 7.c
  - Instead the recoveryAccessor is added to the `ChimeraTK::DeviceModule::writeAfterOpen` list

### VariableNetworkNode ###

- Implements the decision tree mentioned in 8.b in `VChimeraTK::ariableNetworkNode::hasInitialValue()`

### Application ###
(This section refers to the class `ChimeraTK::Application`, not to the user-defined application.)

- Need to implement 7.b:
  - In `ChimeraTK::Application::run()`, after the calls to `ChimeraTK::ApplicationModule::prepare()` all module outputs are checked
  - If a variable has been written during `ChimeraTK::ApplicationModule::prepare()`, the hasInitialValue flag is set in the corresponding `VariableNetworkNode`.
  - The VersionNumber is used to determine if a variable has been written or not (cf. 2.)


## Known bugs ##

### NDRegisterAccessor implementations ###

- 1. is not implemented for Device implementations (only the `UnidirectionalProcessArray` is correct at the moment).
- 2. is not implemented for Device implementations (only the `UnidirectionalProcessArray` is correct at the moment).

### ExceptionHandlingDecorator ###

- It waits until the device is opened, but not until after the initialisation is done.

### Documentation ###

- Documentation of ControlSystemAdapter should mention that implementations must take care about 5.
- Documentation for ApplicationModule should mention 7.