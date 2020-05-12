Technical specification: TransferElement {#spec_TransferElement}
========================================

> **DRAFT VERSION, WRITE-UP IN PROGRESS!**


## Introduction ##

This documnent is currently still **INCOMPLETE**!

## A. Definitions ##

* 1. A <i>process variable</i> is the logical entity which is accessed through the TransferElement. Outside this document, it is sometimes also called register.
  * 1.1 A process variable can be read-only, write-only or read-write (bidirectional).
  * 1.2 A process variable as a data type.
  * 1.3 A process variable has a fixed <i>number of elements</i> and a <i>number of channels</i>.
    * 1.3.1 For scalars, both the number of elements and channels are 1.
    * 1.3.2 For 1D arrays, the number of channels is 1.

* 2. A <i>device</i> is the logical entity which owns the <i>process variable</i>. The device can be a piece of hardware, another application or even the current application.
  * 2.1 The first two cases (piece of hardware and another application) are considered identical, since just different <i>backends</i> are used for the communication.
  * 2.2 The third case (current application) is when using the ChimeraTK::ControlSystemAdapter::ProcessArray e.g. in ApplicationCore.
  * 2.3 The application-side behavior of all three cases is identical. The requirements for the implementation are slightly different in some aspects. This will be mentioned where applicable.

* 3. A <i>transfer</i> is the exchange of data between the application and the device, using a transfer protocol which determines the technical implementation. The protocol used for the transfer is determined by the backend and hence all details about the protocol are abstracted and not visible by the application.

* 4. An <i>operation</i> is the action taken by the application to read or write data from or to a device. An operation is related to a transfer, yet it is to be distinguished. The transfer can e.g. be initiated by the device, while the operation is always initiated by the application.

* 5. The <i>application buffer</i> (sometimes also called user buffer outside this document) is referring to the buffer containing the data and meta data, which is accessible to the application.
  * 5.1 It can be accessed through the following functions:
    * ChimeraTK::NDRegisterAccessor::accessData() / ChimeraTK::NDRegisterAccessor::accessChannel()
    * ChimeraTK::TransferElement::getVersionNumber()
    * ChimeraTK::TransferElement::dataValidity()
  * 5.2 If not stated otherwise, the term <i>application buffer</i> refers to **all** components of the buffer.
  * 5.3 The content of the buffer is in read operations and transferred to the device in write operations.
  * 5.4 The content of the buffer can always be modified by the application.(*)

* 6. Placeholders are used to summarise various function names:
  * 6.1 xxxYyy(), public, operations called by the application
    * ChimeraTK::TransferElement::read()
    * ChimeraTK::TransferElement::readNonBlocking()
    * ChimeraTK::TransferElement::readLatest()
    * ChimeraTK::TransferElement::write()
    * ChimeraTK::TransferElement::writeDestructively()
  * 6.2 preXxx(), private
    * ChimeraTK::TransferElement::preRead()
    * ChimeraTK::TransferElement::preWrite()
  * 6.3 doPreXxx(), virtual
    * ChimeraTK::TransferElement::doPreRead()
    * ChimeraTK::TransferElement::doPreWrite()
  * 6.4 xxxTransferYyy(), private
    * ChimeraTK::TransferElement::readTransfer()
    * ChimeraTK::TransferElement::readTransferNonBlocking()
    * <strike>ChimeraTK::TransferElement::readTransferLatest()</strike>
    * ChimeraTK::TransferElement::writeTransfer()
    * ChimeraTK::TransferElement::writeTransferDestructively()
  * 6.5 doXxxTransferYyy()
    * ChimeraTK::TransferElement::doReadTransferSynchonously(), pure virtual (*)
    * <strike>ChimeraTK::TransferElement::doReadTransferNonBlocking()</strike>
    * <strike>ChimeraTK::TransferElement::doReadTransferLatest()</strike>
    * ChimeraTK::TransferElement::doWriteTransfer(), pure virtual
    * ChimeraTK::TransferElement::doWriteTransferDestructively(), virtual
  * 6.6 postXxx(), private
    * ChimeraTK::TransferElement::postRead()
    * ChimeraTK::TransferElement::postWrite()
  * 6.7 doPostXxx(), virtual
    * ChimeraTK::TransferElement::doPostRead()
    * ChimeraTK::TransferElement::doPostWrite()
   
* 7. Private and virtual functions
  * 7.1 The private functions preXxx(), xxxTransferYyy(), and postXxx() implement common, decorating functionality like exception handling in the TransferElement base class
  * 7.2 They internally call their virtual counterparts which start with 'do'
  * 7.3 The virtual do-functions are the actual implementations of the transfer or pre/post action, which are specific for each backend, decorator etc.

### (*) Comments ###

* 5.4 The buffer is accessed by the operatons and must not be changed at this time. As transfer elements are not thread safe (B.1) this means the application will either perform an operation or otherwise change the buffer ad libitum.
* 6.5 doReadTransferSynchonously() is currently called doReadTransfer(). This should be renamed.

## B. Behavioural specification ##

* 1. TransferElements are not thread safe

* 2. Data types [TODO]

* 3. Modes of transfers

  * 3.1 Read operations:
    * 3.1.1 The flag ChimeraTK::AccessMode::wait_for_new_data determines whether the transfer is initiated by the device side (flag is set) or not.
    * 3.1.2 If ChimeraTK::AccessMode::wait_for_new_data is not set, read operations
      * obtain the <i>current</i> value of the process variable (if possible/applicable by synchronously communicating with the device),
      * have no information whether the value has changed, and
      * behave identical whether read(), readNonBlocking() or readLatest() is called, and readNonBlocking() and readLatest() always return true.
    * 3.1.3 If ChimeraTK::AccessMode::wait_for_new_data is set,
      * read() blocks until new data has arrived,
      * readNonBlocking() does not block and instead returns whether new data has arrived or not, and
      * readLatest() is merely a convenience function which calls readNonBlocking() until no more new data is available.
      
  * 3.2 Write operations
    * 3.2.1 do not distingish on which end the transfer is initiated. The API allows for application-initiated transfers and is compatible with device-initiated transfers as well.
      * It is guaranteed that the application buffer is still intact after the write operation.
    * 3.2.2 can optionally be "destructively", which allows the implementation to destroy content of the application buffer in the process.
      * Applications can allow this optimisation by using writeDestructively() instead of write().
      * The optimisation is still optional, backends are allowed to not make use of it. In this case, the content of the application buffer will be intact after writeDestructively(). Applications still are not allowed to use the content of the application buffer after writeDestructively().

* 4. Stages of an operation initiated by calling xxxYyy()
  * 4.1 preXxx(): calls doPreXxx() of the implementation to allow preparatory work before the actual transfer. doPreXxx() can be empty if nothing is to be done (*)
    * 4.1.1 preXxx() is part of the operation, not of the actual transfer. In case of reads with AccessMode::wait_for_new_data the transfer is asynchonousy initiated by the device and not connected to the operation. Hence backend implementations usually have an empty doPreWrite(), but decorator-like implementations still can use it to execute preparatory tasks.
  * 4.2 xxxTransferYyy():
    * 4.2.1 readTransfer()
      * If wait_for_new_data is set, it waits until new data has been received and returns
      * If wait_for_new_data is not set, it calls doReadTransferAsynchrously()
    * 4.2.2 readTransferNonBlocking()
      * If wait_for_new_data is set, it returns immediately with the information whether new data has been received
      * If wait_for_new_data is not set, it calls doReadTransferAsynchrously() and returns true
    * 4.2.3 writeTransferYyy() calls the corresonding doWriteTransferYyy()
    * Transfer implementations do not change the application buffer
  * 4.3 postXxx(): calls doPostXxx() of the implementation to allow follow-up work after the actual transfer.
    * 4.3.1 doPostRead() is the only place where the application buffer may be changed.

* 5. preXxx() and postXxx(), resp. doPreXxx() and doPostXxx(), are called always in pairs. (*)
  * 5.1 This holds even if exceptions (both ChimeraTK::logic_error and ChimeraTK::runtime_error, and also boost::thread_interrupted and boost::numeric::bad_numeric_cast) are thrown (see 6).
  * 5.2 The implementations of preXxx() and postXxx() ignore duplicate calls, such that a call to doPreXxx() is never followed by another call to doPreXxx() before doPostXxx() has been called, and vice versa.
  
* 6. Exceptions thrown in preXxx() or xxxTransferYyy() are caught and delayed until postXxx() by the framework. This ensures that preXxx() and postXxx() are always called in pairs.
  * 6.1 If in preXxx() an exception is thrown, the corresponding xxxTransferYyy() is not called, instead directly postXxx() is called.

* 7. Return values of xxxTransferYyy():
  * 7.1 readTransferNonBlocking() returns whether new data has been received (see 4.2.2)
  * 7.2 writeTransfer() and writeTransferDestructively() return whether data has been lost. If it returns true, some data was rejected in the process of the transfer. It can be either the data that should have been written in the current transfer, or some older data was overwritten.
  * 7.3 The return value is passed on to the postXxx() function call (via hasNewData resp. dataLost), to allow the doPostXxx() implementations to decide the right actions.
  * 7.4 In case of an exception in either preXxx() or xxxTransferYyy(), postXxx() receives instead the information that the transfer didn't sucessfully take place (hasNewData = false resp. dataLost = true).
  
* 8. Read operations with AccessMode::wait_for_new_data:
  * 8.1 Since the transfer is initiated by the device side in this case, the transfer is asynchronous to the read operation.
  * 8.2 The backend fills any received values into a queue, from which the read()/readNonBlocking() operations will obtain the value.
    * 8.2.1 If the queue is full, the last written value will be overwritten.
    * 8.2.2 The backend may fill a ChimeraTK::detail::DiscardValueException to the queue, which has the same effect on the application side as if no entry was filled to the queue. (*)
  * 8.3 Runtime errors like broken connections are reported by the backend by pushing ChimeraTK::runtime_error exceptions into the queue. The exception will then be obtained by the read operation in place of a value.
  * 8.4 Due to the asynchronous nature, it is not possible to obtain a valid value before the first device-initiated transfer took place.
    * Hint: Applications may use an accessor without AccessMode::wait_for_new_data to read the initial value after opening the device.
  * 8.5 The backend ensures consistency of the value with the device, even if data loss may occur on the transport layer. If necessary, a heartbeat mechanism is implemented to correct any inconsistencies at regular intervals.
  * 8.6 When open() is called on the backend(*), all accessors with AccessMode::wait_for_new_data must get an initial value via doReadTransferAsync() and treat it as if it would have been received.
    * 8.6.1 An initial value must also be read when a TransferElement is created when the backend is already opened.

* 9. If one transfer element of a device has seen an exception, all other transfer elements must also throw exceptions until the backend has been recovered (*). As long a ChimeraTK::DeviceBackend::isFunctional() returns false, all transfer elements shall throw when a read/write operation is called.

* 10. TransferGroup
  * 10.1 TransferGroups are only allowed for TransferElements without AccessMode::wait_for_new_data.
  *  [TODO]

* 11. ReadAnyGroup [TODO]

* 12. DataConsistencyGroup [TODO]


### (*) Comments ###

* 4.1 preXxx() is part of the operation, not of the actual transfer. In case of reads with AccessMode::wait_for_new_data the transfer is asynchonousy initiated by the device and not connected to the operation. Hence backend implementations usually have an empty doPreWrite(), but decorator-like implementations still can use it to execute preparatory tasks.

* 5. Reason: It might be that the user buffer has to be swapped out during the transfer (while taking away the ownership of the calling code), and this must be restored in the postXxx action.

* 8.2.2 This allows to discard values inside a continuation of a cppext::future_queue. It is used e.g. by the ControlSystemAdapter's BidirectionalProcessArray. [TBD: It could be replaced by a feature of the cppext::future_queue allowing to reject values in continuations...]

* 8.6 Open can be called again on an already opened backend to start error recovery.

* 9. The backend is usually recovered by calling ChimeraTK::DebiceBackend::open(). But it can also recover itself in the background.

## C. Requirements for all implementations (full and decorator-like) ##

* 1. Other exceptions than ChimeraTK::logic_error, ChimeraTK::runtime_error, boost::thread_interrupted and boost::numeric::bad_numeric_cast are not allowed to be thrown or passed through at any place under any circumstance (unless of course they are guaranteed to be caught before they become visible to the application, like the detail::DiscardValueException). The framework (in particular ApplicationCore) may use "uncatchable" exceptions in some places to force the termination of the application. Backend implementations etc. may not do this, since it would lead to uncontrollable behaviour.
* 2. In doPostXxx no new ChimeraTK::runtime_error or ChimeraTK::logic_error are thrown. *Only* exceptions that were risen in doPreXxx or doXxxTransferYyy, or that were received from the queue (see A.6.3) may be rethrown. This is done by the TransferElement base class in postXxx, so implementations should never actively throw these exceptions in doPostXxx (but decorators must expect exceptions to be thrown by delegated calls).

* 3. boost::thread_interrupted may be thrown at any stage, if TransferElement::interrupt has been called.

* 4. ChimeraTK::runtime_error in principle are recoverable. A later call to the same function must be able to succeed (for instance if a network outage has been resolved).
   ChimeraTK::runtime_erro may only be thrown in 
  * 4.1 doXxxTransferYyy(). It is the only exception that can occur in this function (except for boost::thread_interrupted)
  * 4.2 ChimeraTK::TransferElement::isReadable(), ChimeraTK::TransferElement::isWriteable() or ChimeraTK::TransferElement::isReadOnly() if there is no map file or such, and it needs to be determined from the running device.

* 5. boost::numeric::bad_numeric_cast may only be thrown in
  * 5.1 ChimeraTK::TransferElement::doPreWrite()
  * 5.2 ChimeraTK::TransferElement::doPostRead(), but only if hasNewData == true
  * 5.3 This exception can in principle be avoided by chosing a user data type that can fit the data without overflow when reading, or small enough so the process variable can hold the data without overflow when writing. New implementations should not throw this exception. Instead a check should be done in the constructor and a ChimeraTK::logic_error should be thrown (see 6.2.4) (*).

* 6. ChimeraTK::logic_error must follow strict conditions.
  * 6.1 logic_errors must be deterministic. They must always be avoidable by calling the corresponding test functions before executing a potentially failing action (*), and must occur if the logical condition is not fulfilled and the function is called anyway.
  * 6.2 Any logic_error must be thrown as early as possible(*). They are thrown **if and only if** one of the following conditions are met:
    * 6.2.1 A register does not exists my that name
      * 6.2.1.1 Can be checked in the catalogue (*)
      * 6.2.1.2 Thrown in the constructor if possible (*), otherwise in doPreXxx(), isReadable(), isWriteable() and isReadOnly().
    * 6.2.2 The size or dimension of the requested TransferElement is too large (*)
      * 6.2.2.1 Can be checked in the catalogue.
      * 6.2.2.2 Thrown in the constructor if possible, otherwise in doPreXxx().
    * 6.2.3 The wrong ChimeraTK::AccessMode flags are provided
      * 6.2.3.1 Can be checked in the catalogue.
      * 6.2.3.2 Thrown in the constructor if possible, otherwise in doPreXxx().
    * 6.2.4 The requested user data type is to small to hold the data without range overflow when reading, or too big when writing (*)
      * 6.2.4.1 ToDo: cannot be checked in the catalogue to a sufficient degree
      * 6.2.4.2 Thrown in the constructor if possible, otherwise in doPreXxx()
    * 6.2.5 A read/write operation is started while the backend is still closed
      * 6.2.5.1 Check with ChimeraTK::DeviceBackend::isOpen().
      * 6.2.5.2 Thrown in doPreXxx() (*)
    * 6.2.6 A read operation is executed on a transfer element that cannot be read
      * 6.2.6.1 Check with TransferElement::isReadable().
      * 6.2.6.2 Thrown in doPreRead()
    * 6.2.7 A write operation is executed on a transfer element that cannot be written
      * 6.2.7.1 Check with TransferElement::isWriteable() or TransferElement::isReadOnly()
      * 6.2.7.2 Thrown in doPreWrite()
  * 6.3 Two remarks about how to follow these rules:
    * 6.3.1 If there is an error in the map file and the backend does not want to fail on this in backend creation, the register must be hidden from the catalogue.
    * 6.3.2 isReadable(), isWriteable() and isReadOnly() might have to check for the existance of the register from a running backend (in case there is no map file). This implies that these calls may throw ChimeraTK::logic_error and ChimeraTK::runtime_error exceptions.

### (*) Comments ###
* 5.3 If there is no map file or such, the information about the target data types must be deretmined from the device. This cannot be done in the constructor if the backend is still closed. In this case the overflow check can happen at runtime and result in a boost::numeric::bad_numeric_cast.
* 6.1 Test functions do not yet exist for everything. This needs to be changed!
* 6.2 Especially no logic_error must be thrown in doXxxTransferYyy() or doPostXxx(). All tests for logical consistency must be done in doPreXxx() latest.
* 6.2.1.1 It is legal to provide "hidden" registers not present in the catalogue, but a register listed in the catalogue must always work.
* 6.2.1.2 If a backend cannot decide the existence of a register in the accessor's constructor (because there is no map file or such, and the backend might be closed), it needs to check the presence later. If the information is available in the constructor, the check has to be done there.
* 6.2.2 This also includes that the offset in a one dimensional case is so large that there are not enought elements left to provide the requested data.
* 6.2.4 Some backends currenty throw a boost:::numeric::bad_numeric_cast instead as described in 5.
* 6.2.5.2 The generic tests if a backend is opened, or if an accessor readable or writeable are intentionally not implemented in TransferElement because they would invovle additional virtual function calls. To avoid these each implementation has to implement the checks in doPreXxx().

## D. Requirements for full implementations (e.g. in backends) ##

* 1. If ChimeraTK::AccessMode::wait_for_new_data is specified:
  * 1.1 ChimeraTK::TransferElement::readQueue is initialised in the constructor. [TBD: Allow setting the queue length through the public API?]
  * 1.2 ChimeraTK::TransferElement::readQueue is pushed whenever new data has arrived. If important for the implementation, the return value of cppext::future_queue::push_overwirte() will tell whether there has been discarded (*).
  
  * 1.3 In case an exception is detected during an asychronous transfer (for instance in a separate thread), the exception must be stored on the ChimeraTK::TransferElement::readQueue. The TransferFuture will then make sure that the exception is properly rethrown in postRead (just like for synchronous transfers).

### (*) Comments ###

1.2 Either the currently pushed data or older data on the queue might be discarded. In any case there will be one call less to ChimeraTK::TransferElement::doPostRead(), because the number of entries in the queue could not be increased because it was full.

## E. Requirements for decorator-like implementations ##

* 1. If ChimeraTK::AccessMode::wait_for_new_data is specified, ChimeraTK::TransferElement::readQueue is initialised in the constructor with a copy of the readQueue of the target TransferElement.
  * 1.1 Decorator-like implementations with multiple targets must provide a readQueue e.g. by using cppext::future_queue::when_any() or cppext::future_queue::when_all().

* 2. All functions doPreXxx, doXxxTransferYyy and doPostXxx must delegate to their non-do counterparts (preXxx, xxxTransferYyy and postXxx). Never delegate to the do... of the target implementation functions directly.

* 3. If a function of the same instance should be called, e.g. if doWriteTransferDestructively() should redirect to doWriteTransfer(), or if doPostRead() should call doPostRead() of a base class, call to do-version of the function. This is merely to avoid code duplication, hence the surrounding logic of the non-do function is not wanted here.

* 4. Decorators must merely delegate doXxxTransferYyy, never add any functionalty there. Reason: TransferGroup might effectively bypass the decorator implementation of these functions.

* 5. All real decorators are in fact decorators of NDRegisterAccessors<USER_TYPE>. Each decoration level contains one NDRegisterAccessors<USER_TYPE>::buffer_2D. The
     decorator implementation must make sure that its buffer is correctly synchronised with the target's buffer.(*)

### (*) Comments ###

* 5. The NDRegisterAccessorDecorator base class already contains an implementation which does this in doPreXxx() and doPostXxx(). You usually call it from the Decorators implementation, as mentioned in 3.


## F. Implementation in the framework (e.g. class TransferElement itself)

**Note:** This section is biased by the current implementation and mostly lists parts that needs to be changed!

* 1. The TransferFuture will be kept as is for now to provide backwards compatibility and as a helper to the TransferElement implementation.

* 2. TransferElement

  * 2.1 ChimeraTK::TransferElement::readAsync() is deprecated
  
  * 2.2 ChimeraTK::TransferElement::activeFuture is created in the first call to read(), readNonBlockig() or readAsync() from ChimeraTK::TransferElement::readQueue if ChimeraTK::AccessMode::wait_for_new_data is set.
  
  * 2.3 read(), readNonBlocking() first check whether ChimeraTK::AccessMode::wait_for_new_data is set.
    * 2.3.1 If no, the sequence of preRead(), readTransfer() and postRead() is executed (cf. 3).
    * 2.3.1 If yes, implement read() as a sequence of preRead() and activeFuture.wait(), resp. readNonBlocking() as a sequence of preRead(), followed by activeFuture.wait() only if activeFuture.hasNewData() == true.
  
  * 2.4 readLatest() is directly calling readNonBlocking() in a loop.
  
  * 2.5 Add function ChimeraTK::detail::getFutureQueueFromTransferElement(), replacing ChimeraTK::detail::getFutureQueueFromTransferFuture().
  
* 3. ReadAnyGroup

  * 3.1 Use the new ChimeraTK::detail::getFutureQueueFromTransferElement().


* 4. ApplicationCore

  * 4.1 For the TestableModeAccessorDecorator: TransferElement::transferFutureWaitCallback() does no longer exist. Instead preRead() will now be called. In case of readAny, multiple calls to preRead() on different accessors will be made on the first call to readAny - the decorator has to deal with that properly.


