Technical specification: TransferElement {#spec_TransferElement}
========================================

> **DRAFT VERSION, WRITE-UP IN PROGRESS!**


## Introduction ##

This documnent is currently still **INCOMPLETE**!

### Definitions ###

* A <i>process variable</i> is the logical entity which is accessed through the TransferElement. Outside this document, it is sometimes also called register.
  * A process variable can be read-only, write-only or read-write (bidirectional).
  * A process variable as a data type.
  * A process variable has a fixed <i>number of elements</i> and a <i>number of channels</i>.
    * For scalars, both the number of elements and channels are 1.
    * For 1D arrays, the number of channels is 1.

* A <i>device</i> is the logical entity which owns the <i>process variable</i>. The device can be a piece of hardware, another application or even the current application.
  * The first two cases (piece of hardware and another application) are considered identical, since just different <i>backends</i> are used for the communication.
  * The third case (current application) is when using the ChimeraTK::ControlSystemAdapter::ProcessArray e.g. in ApplicationCore.
  * The application-side behavior of all three cases is identical. The requirements for the implementation are slightly different in some aspects. This will be mentioned where applicable.

* A <i>transfer</i> is the exchange of data between the application and the device, using a transfer protocol which determines the technical implementation. The protocol used for the transfer is determined by the backend and hence all details about the protocol are abstracted and not visible by the application.

* An <i>operation</i> is the action taken by the application to read or write data from or to a device. An operation is related to a transfer, yet it is to be distinguished. The transfer can e.g. be initiated by the device, while the operation is always initiated by the application.

* The <i>application buffer</i> (sometimes also called user buffer outside this document) is referring to the buffer containing the data and meta data, which is accessible to the application.
  * It can be accessed through the following functions:
    * ChimeraTK::NDRegisterAccessor::accessData() / ChimeraTK::NDRegisterAccessor::accessChannel()
    * ChimeraTK::TransferElement::getVersionNumber()
    * ChimeraTK::TransferElement::dataValidity()
  * If not stated otherwise, the term <i>application buffer</i> refers to **all** components of the buffer.
  * The content of the buffer is in read operations and transferred to the device in write operations.
  * The content of the buffer can always be modified by the application.

* Placeholders are used to summarise various function names:
  * xxxYyy()
    * ChimeraTK::TransferElement::read()
    * ChimeraTK::TransferElement::readNonBlocking()
    * ChimeraTK::TransferElement::readLatest()
    * ChimeraTK::TransferElement::write()
    * ChimeraTK::TransferElement::writeDestructively()
  * preXxx()
    * ChimeraTK::TransferElement::preRead()
    * ChimeraTK::TransferElement::preWrite()
  * doPreXxx()
    * ChimeraTK::TransferElement::doPreRead()
    * ChimeraTK::TransferElement::doPreWrite()
  * xxxTransferYyy()
    * ChimeraTK::TransferElement::readTransfer()
    * <strike>ChimeraTK::TransferElement::readTransferNonBlocking()</strike>
    * <strike>ChimeraTK::TransferElement::readTransferLatest()</strike>
    * ChimeraTK::TransferElement::writeTransfer()
    * ChimeraTK::TransferElement::writeTransferDestructively()
  * doXxxTransferYyy()
    * ChimeraTK::TransferElement::doReadTransfer()
    * <strike>ChimeraTK::TransferElement::doReadTransferNonBlocking()</strike>
    * <strike>ChimeraTK::TransferElement::doReadTransferLatest()</strike>
    * ChimeraTK::TransferElement::doWriteTransfer()
    * ChimeraTK::TransferElement::doWriteTransferDestructively()
  * postXxx()
    * ChimeraTK::TransferElement::postRead()
    * ChimeraTK::TransferElement::postWrite()
  * doPostXxx()
    * ChimeraTK::TransferElement::doPostRead()
    * ChimeraTK::TransferElement::doPostWrite()
  

## A. Behavioural specification ##

* 1. Data types [TODO]

* 2. Modes of transfers

  * 2.1 Read operations:
    * 2.1.1 The flag ChimeraTK::AccessMode::wait_for_new_data determines whether the transfer is initiated by the device side (flag is set) or not.
    * 2.1.2 If ChimeraTK::AccessMode::wait_for_new_data is not set, read operations
      * obtain the <i>current</i> value of the process variable (if possible/applicable by synchronously communicating with the device),
      * have no information whether the value has changed, and
      * behave identical whether read(), readNonBlocking() or readLatest() is called, and readNonBlocking() and readLatest() always return true.
    * 2.1.3 If ChimeraTK::AccessMode::wait_for_new_data is set,
      * read() blocks until new data has arrived,
      * readNonBlocking() does not block and instead returns whether new data has arrived or not, and
      * readLatest() is merely a convenience function which calls readNonBlocking() until no more new data is available.
      
  * 2.2 Write operations
    * 2.2.1 do not distingish on which end the transfer is initiated. The API allows for device-initiated transfers and is compatible with receiver-initiated transfers as well.
    * 2.2.2 can optionally be "destructively", which allows the implementation to destroy content of the application buffer in the process.
      * Applications can allow this optimisation by using writeDestructively() instead of write().
      * The optimisation is still optional, backends are allowed to not make use of it. In this case, the content of the application buffer will be intact after writeDestructively(). Applications still are not allowed to use the content of the application buffer after writeDestructively().

* 3. Stages of an operation initiated by calling xxxYyy()
  * 3.1 preXxx(): calls doPreXxx() of the implementation to allow preparatory work before the actual transfer. Might be empty, if nothing is to be done.
    * 3.1.1 In case of reads with AccessMode::wait_for_new_data, preRead() and hece doPreRead() is not called before the actual transfer due to its asynchronous nature (initiated by the device). Hence backend implementations usually will do nothing there, but decorator-like implementations still can use it to execute preparatory tasks.
  * 3.2 xxxTransferYyy(): This stage only exists for writes, and for reads without AccessMode::wait_for_new_data.
    * 3.2.1 Calls doWriteTransferYyy() resp. doReadTransfer(), which performs the actual transfer, but does not yet change the application buffer.
  * 3.3 postXxx(): calls doPostXxx() of the implementation to allow follow-up work after the actual transfer.
    * 3.3.1 doPostRead() is the only place where the application buffer may be changed.

* 4. preXxx() and postXxx(), resp. doPreXxx() and doPostXxx(), are called always in pairs. (*)
  * 4.1 This holds even if exceptions (both ChimeraTK::logic_error and ChimeraTK::runtime_error, and also boost::thread_interrupted and boost::numeric::bad_numeric_cast) are thrown (see 4).
  * 4.2 The implementations of preXxx() and postXxx() ignores duplicate calls, such that a call to doPreXxx() is never followed by another call to doPreXxx() before doPostXxx() has been called, and vice versa.
  
* 5. Exceptions thrown in preXxx() or xxxTransferYyy() are caught and delayed until postXxx() by the framework. This ensures that preXxx() and postXxx() are always called in pairs.
  * 5.1 If in preXxx() an exception is thrown, the corresponding xxxTransferYyy() is not called, instead directly postXxx() is called.

* 6. Return values of xxxTransferYyy():
  * 6.1 readTransferNonBlocking() returns whether there has been new data. It returns true, if readTransfer() would not have blocked, and false otherwise.
  * 6.2 writeTransfer() and writeTransferDestructively return whether there has been data lost. If it returns true, some data was rejected in the process of the transfer. It can be either the data that should have been written in the current transfer, or some older data was overwritten.
  * 6.3 The return value is passed on to the postXxx() function call (via hasNewData resp. dataLost), to allow the doPostXxx() implementations to decide the right actions.
  * 6.4 In case of an exception in either preXxx() or xxxTransferYyy(), postXxx() receives instead the information that the transfer didn't sucessfully take place (hasNewData = false resp. dataLost = true).
  
* 7. Read operations with AccessMode::wait_for_new_data:
  * 7.1 Since the transfer is initiated by the device side in this case, the transfer is asynchronous to the read operation.
  * 7.2 The backend fills any received values into a queue, from which the read()/readNonBlocking() operations will obtain the value.
    * 7.2.1 If the queue is full, the last written value will be overwritten.
    * 7.2.2 The backend may fill a ChimeraTK::detail::DiscardValueException to the queue, which will have the same effect as no entry filled to the queue. (*)
  * 7.3 Runtime errors like broken connections are reported by the backend by pushing ChimeraTK::runtime_error exceptions into the queue. The exception will then be obtained by the read operation in place of a value.
  * 7.4 Due to the asynchronous nature, it is not possible to obtain a valid value before the first device-initiated transfer took place.
    * Hint: Applications may use an accessor without AccessMode::wait_for_new_data to read the initial value after opening the device.
  * 7.5 The backend ensures consistency of the value with the device, even if data loss may occur on the transport layer. If necessary, a heartbeat mechanism is implemented to correct any inconsistencies at regular intervals.
  
* 7. TransferGroup [TODO]

* 8. ReadAnyGroup [TODO]

* 9. DataConsistencyGroup [TODO]


### (*) Comments ###

* 4. Reason: It might be that the user buffer has to be swapped out during the transfer (while taking away the ownership of the calling code), and this must be restored in the postXxx action.

* 7.2.2 This allows to discard values inside a continuation of a cppext::future_queue. It is used e.g. by the ControlSystemAdapter's BidirectionalProcessArray. [TBD: It could be replaced by a feature of the cppext::future_queue allowing to reject values in continuations...]


## B. Requirements for all implementations (full and decorator-like) ##

* Other exceptions than ChimeraTK::logic_error, ChimeraTK::runtime_error, boost::thread_interrupted and boost::numeric::bad_numeric_cast are not allowed to be thrown or passed through at any place under any circumstance (unless of course they are guaranteed to be caught before they become visible to the application, like the detail::DiscardValueException). The framework (in particular ApplicationCore) may use "uncatchable" exceptions in some places to force the termination of the application. Backend implementations etc. may not do this, since it would lead to uncontrollable behaviour.


* In the constructor of a TransferElement implementation, *only* ChimeraTK::logic_errror may be thrown.
* In doPreXxx, *only* ChimeraTK::logic_error may be thrown. In doPreWrite, boost::numeric::bad_numeric_cast can in addition be thrown.
* In doXxxTransferYyy, *only* ChimeraTK::runtime_error may be thrown
* In doPostXxx, *only* exceptions that were risen in doPreXxx or doXxxTransferYyy, or that were received from the queue (see A.6.3) may be rethrown. This is done by the TransferElement base class in postXxx, so implementations should never actively throw these exceptions in doPostXxx (but decorators must expect exceptions to be thrown by delegated calls). In doPostRead, boost::numeric::bad_numeric_cast may in addition be thrown (also directly), but only if hasNewData == true.
* In addition to the above rules, boost::thread_interrupted may be thrown at any stage, if TransferElement::interrupt has been called. This exception will also be delayed and rethrown in doPostXxx by the TransferElement base class.

* ChimeraTK::logic_errors must follow strict conditions. Any logic_error must be thrown as early as possible. Also logic_erors must always be avoidable by calling the corresponding test functions before executing a potentially failing action. (Note: test functions do not yet exist for everything, this needs to be changed!). In particular:
  * in the accessor's constructor, a logic_error must only be thrown if:
    * no register exists by that name in the catalogue (note: it is legal to provide "hidden" registers not present in the catalogue, but a register listed in the catalogue must always work)
  * in doPreRead(), a logic_error must be thrown if and only if:
    * ChimeraTK::DeviceBackend::isOpen() == false
    * ChimeraTK::TransferElement::isReadable() == false or throws
  * in doPreWrite(), a logic_error must be thrown if and only if:
    * ChimeraTK::DeviceBackend::isOpen() == false
    * ChimeraTK::TransferElement::isWriteable() == false or throws
  * To comments about how to follow these rules:
    * If there is an error in the map file and the backend does not want to fail on this in backend creation, the register must be hidden from the catalogue.
    * If a backend cannot decide the existence of a register in the accessor's constructor (because there is no map file or such, and the backend might be closed), it needs to check the presence also e.g. in isReadable(), isWriteable() and isReadOnly(). This implies that these calls may throw ChimeraTK::logic_error and ChimeraTK::runtime_error exceptions.


## C. Requirements for full implementations (e.g. in backends) ##

* If ChimeraTK::AccessMode::wait_for_new_data is specified:
  * ChimeraTK::TransferElement::readQueue is initialised in the constructor. [TBD: Allow setting the queue length through the public API?]
  * ChimeraTK::TransferElement::readQueue is pushed whenever new data has arrived. If important for the implementation, the return value of cppext::future_queue::push() will tell whether there will be a corresponding call to ChimeraTK::TransferElement::doPostRead() (if the queue is full, there will be no call).
  
  * In case an exception occurs during an asychronous transfer (after a call to readTransferAsync), the exception must be stored on the cppext::future_queue which has been returned with the TransferFuture by doReadTransferAsync. The TransferFuture will then make sure that the exception is properly stored and rethrown in postRead (just like for synchronous transfers).

## D. Requirements for decorator-like implementations ##

* If ChimeraTK::AccessMode::wait_for_new_data is specified, ChimeraTK::TransferElement::readQueue is initialised in the constructor with a copy of the readQueue of the target TransferElement. Decorator-like implementations with multiple targets must provide a readQueue e.g. by using cppext::future_queue::when_any() or cppext::future_queue::when_all().

* All functions doPreXxx, doXxxTransferYyy and doPostXxx must delegate to their non-do counterparts (preXxx, xxxTransferYyy and postXxx). Never delegate to the do... of the target implementation functions directly.

* If a function of the same instance should be called, e.g. if doWriteTransferDestructively should redirect to doWriteTransfer or if doPostRead should call doPostRead of a base class, call to do-version of the function. This is merely to avoid code duplication, hence the surrounding logic of the non-do function is not wanted here.

* Decorators must merely delegate doXxxTransferYyy, never add any functionalty there. Reason: TransferGroup might effectively bypass the decorator implementation of these functions.


## E. Implementation in the framework (e.g. class TransferElement itself)

**Note:** This section is biased by the current implementation and mostly lists parts that needs to be changed!

### E.1 TransferFuture

The TransferFuture will be kept as is for now to provide backwards compatibility and as a helper to the TransferElement implementation.

### E.2 TransferElement

* 2.1 ChimeraTK::TransferElement::readAsync() is deprecated

* 2.2 ChimeraTK::TransferElement::activeFuture is created in the first call to read(), readNonBlockig() or readAsync() from ChimeraTK::TransferElement::readQueue if ChimeraTK::AccessMode::wait_for_new_data is set.

* 2.3 read(), readNonBlocking() first check whether ChimeraTK::AccessMode::wait_for_new_data is set.
  * 2.3.1 If no, the sequence of preRead(), readTransfer() and postRead() is executed (cf. 3).
  * 2.3.1 If yes, implement read() as a sequence of preRead() and activeFuture.wait(), resp. readNonBlocking() as a sequence of preRead(), followed by activeFuture.wait() only if activeFuture.hasNewData() == true.

* 2.4 readLatest() is directly calling readNonBlocking() in a loop.

* 2.5 Add function ChimeraTK::detail::getFutureQueueFromTransferElement(), replacing ChimeraTK::detail::getFutureQueueFromTransferFuture().

### E.3 ReadAnyGroup

* 3.1 Use the new ChimeraTK::detail::getFutureQueueFromTransferElement().


### E.4 ApplicationCore

* 4.1 For the TestableModeAccessorDecorator: TransferElement::transferFutureWaitCallback() does no longer exist. Instead preRead() will now be called. In case of readAny, multiple calls to preRead() on different accessors will be made on the first call to readAny - the decorator has to deal with that properly.


