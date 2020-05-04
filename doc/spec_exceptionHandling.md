// put the namespace around the doxygen block so we don't have to give it all the time in the code to get links
namespace ChimeraTK {
/**
\page spec_execptionHandling Technical specification: Exception handling for device runtime errors

<b>DRAFT VERSION, WRITE-UP IN PROGRESS!</b>

\section spec_execptionHandling_changes Recent changes

This section lists bigger recent changes, which might be hard to track due to restructuring the document at the same time. This section will go away once the changes have been reviewed.

- Write never blocks in case of exceptions. The following spec points (with discussion why) were hence <b>removed/replaced</b>. See also the mattermost channel.
  - <strike>Write operations will block immediately until the device has been recovered and the write operation has been completed. [TBD: is this really a good idea? <b>COMMENT</b>: The order of write operations is still not guaranteed through the recovery accessors (which maybe should be changed), and blocking writes has some severe drawbacks. Not only in fan outs but also in normal ApplicationModules blocking writes will prevent propagation of DataValidity flags! Blocking writes might help if a sequence of values is written to the same register - this is not handled by the recovery accessor. But if a handshake register is read back in between the writes, the situation can already be handled properly (check DataValidity flag, restart sequence after recovery). Maybe blocking writes create more probelms then they solve!? On the other hand, how does the application then know that a write() has no effect yet? E.g. a PI controller might wind-up if actuator and sensor are on different devices and the actuator fails. Then again, how is this different a failing actuator hardware without breaking the communication? Some form of a status readback of the actuator again cures the situation. I think I am in favour of "fire-and-forget" writes.].</strike>
    - <strike>Write should not block in case of an exception for the outputs of ThreadedFanOut / TriggerFanOut.</strike>
    - <strike>According to \link spec_initialValuePropagation \endlink, writes in ApplicationModules do not block before the first successful read in the main loop.</strike>

- The order of writes during recovery (through recoveryAccessors) is now guaranteed to be the same as the original writes.

- Direct access to the DataFaultCounter is not necessary. Since the spec says the behavior should be transparent whether a connection is directly made to the device or another ApplicationModule/FanOut is in between, it is sufficient to override the flag returned by ExceptionHandlingDecorator::dataValidity() in case of an exception state. This greatly simplifies the implementation and does not change the behavior.

\section spec_execptionHandling_intro Introduction

Exceptions are handled by ApplicationCore in a way that the application developer does not need to care much about it.

ChimeraTK::runtime_error exceptions are caught by the framework and are reported to the DeviceModule. The DeviceModule handles this exception and periodically tries to open the device. Communication with the faulty device is frozen or delayed until the device is functional again. In case of several devices only the faulty device is frozen. Faulty devices do not prevent the application from starting, only the parts of the application that depend on the fault device are waiting for the device to come up.

Input variables of ApplicationModules which cannot be read due to a faulty device will set and propagate the DataValidity::faulty flag (see also \link spec_dataValidityPropagation \endlink).

When the device is functional, it be (re)initialised by using application-defined initialisation handlers and also recover the last known values of its process variables.

\subsection spec_exceptionHandling_intro_terminology Special terminology used in this document

- An read operation might be "skipped". It means, the operation will not take place at all. Instead, the data is marked as DataValidity::faulty. Note: This term is also used, if the a running operation is interrupted by an exception.
- An read operation might be "frozen". This means, the function called will not return until the fault state is resolved and the operation is executed. Freezing always happens before the actual operation is executed and hence will always act on pre-existing fault states only.
- An write operation might be "delayed". This means, the operation will not be executed immediately and the calling thread continues. The operation will be asynchronosuly executed when the fault state is resolved. Note that the VersionNumber specified in the write operation will be retained and also used for the delayed write operation.
- Whenever a write operation or a call to write() is mentioned, destructive writes via writeDestructively() are included. The destructive write optimisation makes no difference for the exception handling.


\section spec_execptionHandling_behavior A. Behavioural description

- 1. All ChimeraTK::runtime_error exceptions thrown by device register accessors are handled by the framework and are never exposed to user code in ApplicationModules.
  - 1.1 ChimeraTK::logic_error exceptions are left unhandled and will terminate the application. These errors may only occur in the initialisation phase (up to the point where all devices are opened and initialised) and point to a severe configuration error which is not recoverable. (*)
  - 1.2 Exception handling and DataValidity flag propagation is implemented such that it is transparent to a module whether it is directly connected to a device, or whether a fanout or another application module is in between.

- 2. When an exception has been received (thrown by a device register accessor in an ApplicationModule, FanOut etc.):
  - 2.1 The exception status is published as a process variable together with an error message.
    - 2.1.1 The variable Devices/<alias>/status contains a boolean flag whether the device is in an error state
    - 2.1.2 The variable Devices/<alias>/message contains an error message, if the device is in an error state, or an empty string otherwise.
  - 2.2 Read operations will propagate the DataValidity::faulty flag to the owning module / fan out (without changing the actual value):
    - 2.2.1 The normal module algorithm code will be continued, to allow this flag to propagate to the outputs in the same way as if it had been received through the process variable itself (c.f. 1.2).
    - 2.2.2 The DataValidity::faulty flag resulting from the fault state is propagated once, even if the variable had the a DataValidity::faulty flag already set previously for another reason.
    - 2.2.3 readLatest() (including any read operation without AccessMode::wait_for_new_data) will be skipped. The return value will be false (no new data), if the fault flag has been read once already by the same accessor and hence is already propagated (regardless of the type of the first read), true otherwise.
    - 2.2.4 Read operations with AccessMode::wait_for_new_data (read(), readNonBlocking() and readAsync()) will be skipped, if the DataValidity::faulty flag has not yet been propagated by the same accessor (which counts as new data, i.e. readNonBlocking() will return true). Otherwise, it will behave like there is no new data: Blocking operations will be frozen, non-blocking operations will be skipped. When the frozen operation is finally executed, another exception might be thrown, in which case the previously frozen operation is finally skipped.
    - 2.2.5 If the fault state had been resolved in between two read operations (regardless of the type) and the device had become faulty again before the second read is executed, it is not defined whether the second operation will frozen/delayed/skipped (depending on the type) or not. The second operation might behave either like it is a new exception or like the same fault state would still prevail. (*)
  - 2.3 Write operations will be delayed. In case of a fault state (new or persisting), the actual write operation will take place asynchronously when the device is recovering. The same mechanism as used for 3.1.2 is used here, hence the order of write operations is guaranteed across accessors, but only the latest written value of each accessor prevails. (*)
    - 2.3.1 The return value of write() indicates whether data was lost in the transfer. If the write has to be delayed due to an exception, the return value will be true, if a previously delayed and not-yet writen value is discarded in the process, false otherwise.
    - 2.3.2 When the delayed value is finally written to the device during the recovery procedure, it is guaranteed that no data loss happens (writes with data loss will be retried).
    - 2.3.3 It is guaranteed that the write takes place before the device is considered fully recovered again and other transfers are allowed (cf. 3.1).
  - 2.4 In case of exceptions, there is no guaranteed realtime behavior, not even for "non-blocking" transfers. (*)

- 3. The framework tries to resolve an exception state by periodically re-opening the faulty device.
  - 3.1 After successfully re-opening the device, a recovery procedure is executed before allowing any read/write operations from the AppliactionModules and FanOuts again. This recovery procedure involves:
    - 3.1.1 the execution of so-called initialisation handlers (see 3.2), and
    - 3.1.2 restoring all registers that have been written since the start of the application with their latest values. The register values are restored in the same order they were written. (*)
    - 3.1.3 Finally, Devices/<alias>/deviceBecameFunctional is written to inform any module subscribing this variable about the finished recovery. (*)
  - 3.2 Any number of initialisation handlers can be added to the DeviceModule in the user code. Initialisation handlers are callback function which will be executed when a device is opened for the first time and after a device recovers from an exception, before any process variables are written. See DeviceModule::addInitialisationHandler().

- 4. The behavior at application start (when all devices are still closed at first) is similar to the case of a later received exception. The only differences are mentioned in 4.2.
  - 4.1 Even if some devices are initially in a persisting error state, the part of the application which does not interact with the faulty devices starts and works normally.
  - 4.2 Initial values are correctly propagated after a device is opened. See \link spec_initialValuePropagation \endlink. Especially, all read operations (even readNonBlocking/readLatest) will be frozen until an initial value has been received. (*)


\subsection spec_execptionHandling_behavior_comments (*) Comments

- 1.1 In future, maybe logic_errors are also handled, so configuration errors can nicely be presented to the control system. This may be important especially since logic_errors may depend also on the configuration of external components (devices). If e.g. a device is changed (e.g. device is another control system application which has been modified), logic_errors may be thrown in the recovery phase, despite the device had been successfully initialsed previously.

- 2.2.5 Not defining the behavior here avoids a conflict with 1.2 without requiring a complicated implementation which does not block in this case. Implementing this would not present any gain for the application. If there are many exceptions on the same device in a short period of time, the number of faulty data updates seen by the application modules will always depend on the speed the module is attempting to read data (unless we require every exception to be visible to every module, but this will have complex effects, too). It might break consistency of the number of updates sent through different paths in an application, but applications should anyway not rely on that and use a DataConsistencyGroup to synchronise instead. Hence, the implementation will block always if a blocking read sees a known exception

- 2.3 / 3.1.3 If timing is important for write operations (e.g. must not write a sequence of registers too fast), or if multiple values need to be written to the same register in sequence, the application cannot fully rely on the framework's recovery procedure. The framework hence provides the process variable Devices/<alias>/deviceBecameFunctional for each device, which will be written each time the recovery procedure is completed (cf. 3.1.3). ApplicationModules which implement such timed sequence need to receive this variable and restart the entire sequence after the recovery.

- 2.4 Even non-blocking read and write operations are not truely non-blocking, since they are still synchronous. The "non-blocking" guarantee only means that the operation does not block until new data has arrived, and that it is not frozen until the device is recovered. For the duration of the recovery procedure and of course for timeout periods these operations may still block.

- 3.1.2 For some applications, the order of writes may be important, e.g. if firmware expects this. Please note that the VersionNumber is insufficient as a sorting criteria, since many writes may have been done with the same VersionNumber (in an ApplicationModule, the VersionNumber used for the writes is determined by the largest VersionNumber of the inputs).

- 4.2 DataValidity::faulty is initially set by default, so there is no need to propagate this flag initially. To prevent race conditions and undefined behavior, it even needs to be made sure that the flag is not propagated unnecessarily. The behavior of non-blocking reads presents a slight asymmetry between the initial device opening and a later recovery. This will in particular be visible when restarting a server while a device is offline. If a module only uses readLatest()/readNonBlocking() (= read() for poll-type inputs) for the offline device, the module was still running before the server restart using the last known values for the dysfunctional registers (and flagging all outputs as faulty). After the restart, the module has to wait for the initial value and hence will not run until the device becomes functional again. To make this behavior symmetric, one would need to persist the values of device inputs. Since this only affects a corner case in which anyway no usable output is produced, this slight inconsistency is considered acceptable.

\section spec_execptionHandling_high_level_implmentation B. Implementation

\subsection spec_execptionHandling_high_level_implmentation_TransferElement B.0 Requirements to the DeviceAccess TransferElement
Note: This section should be integrated into the TransferElement specification and then removed here. Requirements which are already met by the TransferElement specifciation are not mentioned here.

- 0.1 readAsync() may only be called if AccessMode::wait_for_new_data is set. It will throw a ChimeraTK::logic_error otherwise.
- 0.2 If AccessMode::wait_for_new_data is set, the TransferFuture is initialised in the constructor. All read implementations except readLatest() are then using always the TransferFuture.
- 0.3 readLatest() never uses the TransferFuture. Its implementation is identical to the one read implementation when AccessMode::wait_for_new_data is not set.
- 0.4 readTransferAsync() and doReadTransferAsync() are obsolete and hence removed from the interface.
- 0.5 readAsync() always returns the same TransferFuture in subsequent calls.
- 0.6 TransferFuture::wait() and TransferFuture::hasNewData(), as well as ReadAnyGroup::waitAny(), call TransferElement::preRead() at the beginning (keep in mind that extra calls to preRead() are ignored) before the transferFutureWaitCallback is called. This makes sure that preRead() and postRead() are always called in pairs.
- 0.7 Due to the nature of asynchronous transfers, backends must not expect preRead() to be called before new data arrives and is filled into the cppext::future_queue. Hence, doPreRead() of asynchronous accessor implementations will usually be empty. The call to preRead() is still necessary also for asynchronous transfers, since decorators might have important tasks to be done there.
- 0.8 There is no need to call readAsync() for each read transfer again. If the TransferFuture has been obtained once it can simply be used over and over again. Hence, readAsync() will just return the TransferFuture (which had been created in the constructor already) only. It does not call preRead() - this is done by the TransferFuture (see 0.6), and it doesn't have any side effects. (Maybe it should be renamed into getTransferFuture()).


\subsection spec_execptionHandling_high_level_implmentation_decorator B.1 ExceptionHandlingDecorator

A so-called ExceptionHandlingDecorator is placed around all device register accessors (used in ApplicationModules and FanOuts). It is responsible for catching the exceptions and implementing most of the behavior described in A.2.

- 1.1 A second, undecorated copy of each writeable device register accessor (*) is used as a so-called recoveryAccessor by the ExceptionHandlingDecorator and the DeviceModule. These recoveryAccessor are used to set the initial values of registers when the device is opened for the first time and to recover the last written values during the recovery procedure.
  - 1.1.1 The recoveryAccessor is stored by the DeviceModule with additional meta data in a so-called RecoveryHelper data structure, which contains:
    - the recoveryAccessor itself,
    - the VersionNumber of the (potentially unwritten) data stored in the accessor,
    - an ordering parameter which determines the order of write opereations during recovery.
    - a flag which indicates whether the value in the recoveryAccessor has already been written to data. (*)
  - 1.1.2 Ordering can be done per device (*), hence each DeviceModule has one 64-bit atomic counter which is incremented for each write operation and the is value stored in the ordering parameter for the recoveryAccessor.
  - 1.1.3 The RecoveryHelper object may be accessed only under a lock to prevent concurrent access during recovery. The lock shall be shared to allow concurrent write operations of different registers - only the DeviceModule needs to obtain an exclusive lock during recovery. The lock is obained by the ExceptionHandlingDecorators via DeviceModule::getRecoverySharedLock().
  
- 1.2 In doPreRead()/doPreWrite(), it is checked whether the fault state already prevails (check DeviceModule::deviceHasError while holding the recovery shared lock).
  - 1.2.1 If yes, the actual transfer will be skipped. (cf. 2.2 or 2.3.13)
  - 1.2.2 If the transfer will not be skipped, atomically increment DeviceModule::activeTransfers while still (!) holding the recovery shared lock.
  - 1.2.3 write: The check for a prevailing fault state has to be done without releasing the lock between the write to the recoveryAccessor and the check. (*)
  - 1.2.4 For skipped transfers, none of the pre/transfer/post functions must be delegated to the target accessor.
  - 1.2.5 If an asynchronous read transfer is skipped, a pseudo value needs to be written to the cppext::future_queue of the TransferFuture. This will cause the TransferFuture to be ready immediatly, so postRead() is called (*).

- 1.3 In doPreWrite() the recoveryAccessor with the version number and ordering parameter is updated, and the written flag is cleared.
  - 1.3.1 If the written flag was previously not set, the return value of doWriteTransfer() must be forced to true (data lost).

- 1.4 In doPreRead() certain read operations are frozen in case of a fault state (see A.2.2):
  - 1.4.1 Obtain the recovery lock through DeviceModule::getRecoverySharedLock(), to prevent interference with an ongoing recovery procedure.
  - 1.4.2 Decide, whether freezing is done (don't freeze yet). Freezing is done if one of the following conditions is met:
    - read type is blocking and AccessMode::wait_for_new_data is set, previousReadFailed == true, and DeviceModule::deviceHasError == true (cf. A.2.2.4), or
    - no initial value has been read yet (getCurretVersion() == {nullptr}) and DeviceModule::deviceHasError == true (cf. A.4.2).
  - 1.4.3 Obtain the DeviceModule::errorLock. Only then release the recovery lock. (*)
  - 1.4.4 Wait on DeviceModule::errorIsReportedCondVar.
  - 1.4.5 When the DeviceModule reports the recovery through the condition variable, delegate preRead() and continue with the transfer normally.
  - 1.4.6 If an asynchronous read transfer is frozen, instead of 1.4.3 thorugh 1.4.5 the following actions are executed:
    - 1.4.6.1 Register the asynchronous read transfer with the DeviceModule::asynchronousReadQueue by placing a shared_pointer to this on it.
    - 1.4.6.2 Do not delegate to preRead() and readTransferAsync() - both functions are called by the DeviceModule instead.
    
  
- 1.5 In doPostRead()/doPostWrite():
  - 1.5.1 If there was no exception, set previousReadFailed = false.
  - 1.5.2 If in 1.2.2 the DeviceModule::activeTransfers counter was incremented, atomically decrement it.
  - 1.5.3 In doPostWrite() the recoveryAccessor's written flag is set if the write was successful (no exception thrown; data lost flag does not matter here). (*)
  - 1.5.4 In doPostRead(), if no exception was thrown, end overriding the DataValidity returned by the accessor (cf. 1.6.2).

- 1.6 In doPostRead()/doPostWrite(), any runtime_error exception thrown by the delegated postRead()/postWrite() is caught (*). The following actions are in case of an exception:
  - 1.6.1 The error is reported to the DeviceModule via DeviceModule::reportException().
  - 1.6.2 For readable accessors: the DataValidity returned by the accessor is overridden to faulty until next successful read operation (cf. 1.5.4).
    - 1.6.2.1 The code instantiating the decorator (Application::createDeviceVariable()) has to make sure that the ExceptionHandlingDecorator is "inside" the MetaDataPropagatingRegisterDecorator, so the overriden DataValidity flag in case of an exception is properly propagated to the owning module/fan out.
  - 1.6.3 Action depending on the calling operation:
    - 1.6.3.1 All read operations: The ExceptionHandlingDecorator remembers that it is in an exception state by setting previousReadFailed = true
    - 1.6.3.1 read (push-type inputs): return immediately (*)
    - 1.6.3.2 readNonBlocking / readLatest / read (poll-type inputs): Just return false (no new data). The calling module thread will continue and propagate the DataValidity::faulty flag (cf. 1.6.2).
    - 1.6.3.3 write: Do not block. Write will be later executed by the DeviceModule (see 1.1)


\subsection spec_execptionHandling_high_level_implmentation_deviceModule B.2 DeviceModule

- 2.1 The application always starts with all devices as closed. For each device, the initial value for Devices/<alias>/status is set to 1 and the initial value for Devices/<alias>/message is set to an error that the device has not been opened yet (the message will be overwritten with the real error message if the first attempt to open fails, see 2.3.1).
- 2.2 The DeviceModule takes care that ExceptionHandlingDecorators initally do not perform any read or write operations, but freeze (cf. 1.4). This happens before running any prepare() of an ApplicationModule, where the first write calls to ExceptionHandlingDecorators might be done.
- 2.3 In the DeviceModule thread, the following procedure is executed (in a loop until termination):
  - 2.3.1 The DeviceModule tries to open the device until it succeeds and isFunctional() returns true.
    - 2.3.1.1 If the very first attempt to open the device after the application start fails, the error message of the exception is used to overwrite the content of Devices/<alias>/message. Otherwise error messages of exceptions thrown by Device::open() are not visible.
  - 2.3.2 Obtain lock for accessing recoveryAccessors.
  - 2.3.3 Device is initialised by iterating initialisationHandlers list.
    - 2.3.3.1 If there is an exception, update Devices/<alias>/message with the error message, release the lock and go back to 2.3.1.
  - 2.3.4 All valid recoveryAccessors are written in the same order they were originally written.
    - 2.3.4.1 A recoveryAccessor is considered "valid", if it has already received a value, i.e. its current version number is not {nullptr} any more.
    - 2.3.4.2 If there is an exception, update Devices/<alias>/message with the error message, release the lock and go back to 2.3.1.
  - 2.3.5 The queue of reported exceptions is cleared. (*)
  - 2.3.6 Devices/<alias>/status is set to 0 and Devices/<alias>/message is set to an empty string.
  - 2.3.7 DeviceModule allows ExceptionHandlingDecorators to execute reads and writes again (cf. 2.3.13)
  - 2.3.8 All frozen read operations (cf. 1.4.4) are notified via DeviceModule::errorIsReportedCondVar.
  - 2.3.9 Release lock for recoveryAccessors.
  - 2.3.10 The DeviceModuleThread waits for the next reported exception.
  - 2.3.11 An exception is received.
  - 2.3.12 Devices/<alias>/status is set to 1 and Devices/<alias>/message is set to the first received exception message.
  - 2.3.13 Set DeviceModule::deviceHasError = true under exclusive recovery lock (cf. 1.2). From this point on, no new transfers will be started.
  - 2.3.14 The device module waits until all running read and write operations of ExceptionHandlingDecorators have ended (wait until DeviceModule::activeTransfers == 0). (*)
  - 2.3.15 The thread goes back to 2.3.1 and tries to re-open the device.


\subsection spec_execptionHandling_high_level_implmentation_comments (*) Comments

- 1.6 Remember: exceptions from other phases are redirected to the post phase by the TransferElement base class.

- 1.6.3.1 The freezing is done in doPreRead(), see 1.4.

- 1.1 Possible future change: Output accessors can have the option not to have a recovery accessor. This is needed for instance for "trigger registers" which start an operation on the hardware. Also void registers don't have recovery accessors (once the void data type is supported).

- 1.1.1 The written flag cannot be replaced by comparing the version number of the recoveryAccessor and the version number stored in the RecoveryHelper, because normal writes (without exceptions) would not update the version number of the recoveryAccessor.

- 1.1.2 The ordering guarantee cannot work across DeviceModules anyway. Different devices may go offline and recover at different times. Even in case of two DeviceModules which actually refer to the same hardware device there is no synchronisation mechanism which ensures the recovering procedure is done in a defined order.

- 1.2.5 The cppext::future_queue in the TransferFuture is a notification queue and hence of the type void. So we don't have to "invent" any value. Also this injection of values is legal, since the queue is multi-producer but single-consumer. This means, potentially concurrent injection of values while the actual accessor might also write to the queue is allowed. Also, the application is the only receiver of values of this queue, so injecting values cannot disturb the backend in any way.

- 1.5.3 The written flag for the recoveryAccessor is used to report loss of data. If the loss of data is already reported directly, it should not later be reported again. Hence the written flag is set even if there was a loss of data in this context.

- 1.4.3 The order of locks is important here. The recovery lock prevents the DeviceModule from entering the section 2.3.2 to 2.3.9, which includes the notification through the DeviceModule::errorIsReportedCondVar at 2.3.8. The mutex DeviceModule::errorLock is the mutex used for the condition variable. Since the ExceptionHandlingDecorator obtains it before the DeviceModule can start the notification, it is guaranteed that the decorator does not miss the notification. Note that the DeviceModule::errorLock is not a shared lock, so concurrent ExceptionHandlingDecorator::preRead() will mutually exclude, but the mutex is held only for a short time until errorIsReportedCondVar.wait() is called.

- 1.6.3 The lock excludes that the DeviceModule is between 2.3.2 and 2.3.9. If it is right before, the device is still in fault state and the value written to the recoveryAccessor is guaranteed to be written in 2.3.4. If it is right after, the exception state has already been resolved and the real write transfer will be attempted by the ExceptionHandlingDecorator.

- 2.3.5 The exact place when this is done does not matter, as long as it is done under the lock for the recoveryAccessors.

- 2.3.14 The backend has to take care that all operations, also the blocking/asynchronous reads with "waitForNewData", terminate when an exception is thrown, so recovery can take place (see DeviceAccess TransferElement specification).


\section spec_execptionHandling_known_issues Known issues - OUTDATED (numbers don't even match)

<strike>
- 11.1 In step 2.1: The initial value of deviceError is not set to 1.

- 11.2 In step 2.2.3: is not correctly fulfilled as we are only waiting for device to be opened and don't wait for it to be correctly initialised. The lock 4.2.3 is not implemented at all.

- 11.3 In step 2.3.5: is currently being set before initialisationHandlers and writeAfterOpen.

- 11.4 Check the documentation of DataValidity. ...'Note that if the data is distributed through a triggered FanOut....'

- 11.5 Data validity is currently propagated through the "owner", which conceptually does not always work. A DataFaultCounter needs to be introduced and used at the correct places.

- 11.6 In comment to 1.g: recovery accessors are not optional at the moment.

- 11.7 In 1.c: Currently data is transported even if the "value after construction" is still in.
- 11.8 In 1.i, 6: ThreadedFanout and TriggerFanout do not use non-blocking write because it does not exist yet
- 11.9 In 1.j, 2.5.3: Not implemented like that. The first read blocks, and a special mechanism to propagate the flags is triggered only in the next module.

- 11.10 In 2.3: The device module has a special "first opening" sequence. This is not necessary any more. The "writeAfterOpen" list is obsolete. You can always use the recovery accessors.
- 11.11 In 2.3.4: Recovery accessors are always written. It is not checked whether there is valid data (not "value after construction")
- 11.12 In 2.4.1.1: Write probably re-executed after recovery. This should not happen because the recovery accessor has already done it.
- 11.13 In 2.5.3: The non-blocking read functions always block on exceptions. They should not (only if there is no initial value).
- 11.14 In 2.5.2, 5.1: writeWithoutErrorBlocking is not implemented yet
- 11.15 Asynchronous reads are not working with the current implementation, incl. readAny.

- 11.16 In 3: DeviceAccess : RegisterAccessors throw in doReadTransfer now.
- 11.17 In 4.2.1: reportException does block (should not)
- 11.18 In 4.2.2: blocking wait function does not exist (not needed in current implementation as reportException blocks)

- 11.19 In 5.2.1: Exceptions are caught in doXxxTransfer instead of doPostXxx.
- 11.20 In 5.3.1.2, 5.3.2.1: Decoration of doXxxTransfer does not acquire the lock (which does not even exist yet, see 4.2.3)

- 11.21 In 3.2: Decorators might have to try-catch because they usually can only do their task after calling the delegated postXxx.
- 11.22 In 3.4: The TransferType is not known. Needs to be implemented in TransferElement
- 11.23 In 3.5: PostRead is currently skipped if readNonBlocking or readLatest does not have new data
- 11.24 In 3.6: The waitForNewData calls in the DoocsBackend (using zmq) are currently not interruptible
</strike>


*/

} // end of namespace ChimeraTK
