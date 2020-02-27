Technical specification: propagation of initial values {#spec_initialValuePropagation}
======================================================

> **DRAFT VERSION, WRITE-UP IN PROGRESS!**

## Introduction ##

This document describes how initial values are propagated from the control system persistency layer, from the devices and (if applicable) from application modules into the attached components (control system, devices and other application modules).

This specification goes beyond ApplicationCore. It has impact on other ChimeraTK libraries like DeviceAccess, the ControlSystemAdapter and even backends and adapter implementations.

## Definitions ##

- Initial value: The first valid value of a process variable after application start. This is a logical concept. It is to be distinguished from the (hardcoded) "first value" of the `ChimeraTK::ProcessArray` or any other `ChimeraTK::NDRegisterAccessor` implementation.

## High-level requirements ##

- Initial values must be available to all `ApplicationModule`s at the start of the `mainLoop()`. No call to `read()` etc. is required. This implies that the `mainLoop()` is not started until all initial values are available, including those coming from devices which might potentially be offline.
- Devices must receive the initial values as soon as possible after the device is opened and after the initialisation sequence is executed, but before anything else gets written to the device.
- The control system must receivce the initial values as soon as they are available. The initial value is merely the first value the control system receives for a particular process variable - other variables might have received an update already multiple times before the initial value is recieved.
- Control system variables show the `DataValidity::faulty` flag until they have received the initial value.

## Detailed requirements ##

1. All `ChimeraTK::NDRegisterAccessor` implementations (including but not limited to the `ChimeraTK::ProcessArray`) must have the `DataValidity::faulty` flag set after construction. This ensures, all data is marked as `faulty` as long as no sensible initial values have been propagated.
2. All `ChimeraTK::NDRegisterAccessor` implementations must have initially a `ChimeraTK::VersionNumber` constructed with a `nullptr`, which allows to check whether this variable is still at its "first value" or the initial value propagation already took place.
3. All `ApplicationModules` and similar entities (like `ThreadedFanOut` and `TriggerFanOut`), that store a `DataValidity` directly or indirectly e.g. in form af a counter, must have their internal `DataValidity` flag set to `faulty` after construction. [TBD: What does that mean in case of a counter?]
4. The initial `DataValidity::faulty` flags must not be propagated actively. The first propagated data must be always `ok` and must have a valid value.
5. Control system variables:
  1. Variables with the control-system-to-application direction must be written exactly once at application start by the control system adapter with their initial values from the persistency layer and the `DataValidity::ok`. This must be done before `ApplicationBase::run()` is called. [TBD: Is this last sentence a necessary restiction?]
  2. Initial values of variables with the application-to-control-system direction are written at an undefined time after the `ApplicationBase::run()` has been called. The control system adapter must not expect any specific behaviour.
6. Device variables:
  1. Write accessors need to be written right after the device is opened and the initialisation is done.
  2. Read accessors need to be read after 6.a. [TBD: Is this ordering even possible? It is more like a 'nice to have' and not strictly required.]
7. Outputs of `ApplicationModule`s:
  1. By default, no initial values are propagated.
  2. Initial values can be written in `ApplicationModule::prepare()`.
8. Inputs of `ApplicationModule`s:
  1. Initial values are read before start of `mainLoop()`.
  2. Since not all variables have initial values (see 7.a), the variable model (`VariableNetworkNode`) needs to be checked whether an initial value is present and how it needs to be read. This dependes on the data source type:
    1. control system variable: blocking read
    2. device register without trigger: non-blocking read (even if register is push-type)
    3. device register with trigger (incl. TriggerType::pollingConsumer): blocking read
    4. constant: non-blocking read
    5. application: blocking read only if initial value was provided (see 7.a), otherwise no read
9. Constants:
  1. Values are propagated before the `ApplicationModule` threads are starting.
  2. Special treatment for constants written to devices: They need to be written after the device is opened, see 6.a

## Implementation ##


## Known bugs ##

### ExceptionHandlingDecorator ###

- It waits until the device is opened, but not until after the initialisation is done.