Technical specification: TransferElement {#spec_TransferElement}
========================================

> **DRAFT VERSION, WRITE-UP IN PROGRESS!**

This currently is just a stub with detail snippets.

## Introduction ##

## Unsorted collection of details ###

* preXxx and postXxx are called always in pairs. Reason: It might be that the user buffer has to be swapped out during the transfer (while taking away the ownership of the calling code), and this must be restored in the postXxx action.
* This holds even if exceptions (both ChimeraTK::logic_error and ChimeraTK::runtime_error, and also boost::thread_interrupted and boost::numeric::bad_numeric_cast) are thrown.
* If in preXxx an exception is thrown, the corresponding xxxTransferYyy is not called, instead directly postXxx is called.
* Exceptions thrown in preXxx or xxxTransferYyy are caught and delayed until postXxx by the framework. This ensures that preXxx and postXxx are always called in pairs.
* Other exceptins then ChimeraTK::logic_error, ChimeraTK::runtime_error, also boost::thread_interrupted and boost::numeric::bad_numeric_cast are not allowed to be thrown or passed through at any place under any circumstance (unless of course they are guaranteed to be caught before they become visible to the application, like the detail::DiscardValueException). The framework (in particular ApplicationCore) may use "uncatchable" exceptions in some places to force the termination of the application. Backend implementations etc. may not do this, since it would lead to uncontrollable behaviour.

Requirement for backend/decoractor implementations:

* In the constructor, *only* ChimeraTK::logic_errror must be thrown.
* In doPreXxx, *only* ChimeraTK::logic_errror must be thrown. In doPreWrite, boost::numeric::bad_numeric_cast can in addition be thrown.
* In doXxxTransferYyy, *only* ChimeraTK::runtime_error must be thrown
* In doPostXxx, *only* exceptions that were risen in doPreXxx or doXxxTransferYyy may be rethrown. This is done by the TransferElement base class in postXxx, so implementations should never actively throw these exceptions in doPostXxx (but decorators must expect exceptions to be thrown by delegated calls). In doPostRead, boost::numeric::bad_numeric_cast may in addition be thrown (also directly), but only if hasNewData == true.
* In addition to the above rules, boost::thread_interrupted may be thrown at any phase, if TransferElement::interrupt has been called. This exception will also be delayed and rethrown in doPostXxx by the TransferElement base class.

* ChimeraTK::logic_errors must follow strict conditions. Any logic_error must be thrown as early as possible. Also logic_erors must always be avoidable by calling the corresponding test functions before executing a potentially failing action. (Note: test functions do not yet exist for everything, this needs to be changed!). In particular:
  * in the accessor's constructor, a logic_error must only be thrown if:
    * no register exists by that name in the catalogue (note: it is legal to provide "hidden" registers not present in the catalogue, but a register listed in the catalogue must always work)
  * in doPreRead(), a logic_error must be thrown if and only if:
    * backend->isOpened() == false
    * isReadable() == false
  * in doPreWrite(), a logic_error must be thrown if and only if:
    * backend->isOpened() == false
    * isWriteable() == false
  * To comments about how to follow these rules:
    * If there is an error in the map file and the backend does not want to fail on this in backend creation, the register must be hidden from the catalogue.
    * If a backend cannot decide the existence of a register in the accessor's constructor (because there is no map file or such, and the backend might be closed), it needs to check the presence also e.g. in isReadable() and isWriteable(). TBD: Shall isReadable()/isWriteable() throw in that case or return false?


### Decorators including decorator-like implementations ###

* All functions doPreXxx, doXxxTransferYyy and doPostXxx must delegate to their non-do counterparts (preXxx, xxxTransferYyy and postXxx). Never delegate to the do... of the target implementation functions directly.
* If a function of the same instance should be called, e.g. if doReadTransferLatest should redirect to doReadTransfer or if doPostRead should call doPostRead of a base class, call to do-version of the function. This is merely to avoid code duplication, hence the surrounding logic of the non-do function is not wanted here.
* Decorators must merely delegate doXxxTransferYyy, never add any functionalty there. Reason: TransferGroup might effectively bypass the decorator implementation

