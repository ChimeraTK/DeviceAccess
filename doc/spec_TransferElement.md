Technical specification: TransferElement {#spec_TransferElement}
========================================

> **DRAFT VERSION, WRITE-UP IN PROGRESS!**

This currently is just a stub with detail snippets.

## Introduction ##

## Unsorted collection of details ###

* preXxx and posXxx are called always in pairs. Reason: It might be that the user buffer has to be swapped out during the transfer (while taking away the ownership of the calling code), and this must be restored in the postXxx action.
* This holds even if exceptions (both logic_error and runtime_error) are thrown.
* If in preXxx an exception is thrown, the corresponding xxxTransferYyy is not called, instead directly postXxx is called.
* Exceptions thrown in preXxx or xxxTransferYyy are caught and delayed until postXxx by the framework. This ensures that preXxx and postXxx are always called in pairs.

Requirement for backend/decoractor implementations:

* In the constructor, *only* ChimeraTK::logic_errror must be thrown. No device communication is allowed at that point, as the device may even be closed.
* In doPreXxx, *only* ChimeraTK::logic_errror must be thrown. No device communication is allowed at this point.
* In doXxxTransferYyy, *only* ChimeraTK::runtime_error must be thrown
* In doPostXxx, *only* ChimeraTK::runtime_error which have been delayed from doXxxTransferYyyy must be thrown. No exception must be thrown if doXxxTransferYyy has not been called (because doPreXxx has thrown a ChimeraTK::logic_error). Note that postXxx (without do) may throw a delayed ChimeraTK::logic_error. If doPostXxx delegates to some postXxx implementation, this ChimeraTK::logic_error should be let through. Actively throwing a ChimeraTK::logic_error is not allowed: If the transfer has already been executed the correct exception is the runtime_error, if no transfer was executed the surrounding postXxx will throw.


### Decorators including decorator-like implementations ###

* All functions doPreXxx, doXxxTransferYyy and doPostXxx must delegate to their non-do counterparts (preXxx, xxxTransferYyy and postXxx). Never delegate to the do... of the target implementation functions directly.
* If a function of the same instance should be called, e.g. if doReadTransferLatest should redirect to doReadTransfer or if doPostRead should call doPostRead of a base class, call to do-version of the function. This is merely to avoid code duplication, hence the surrounding logic of the non-do function is not wanted here.
* Decorators must merely delegate doXxxTransferYyy, never add any functionalty there. Reason: TransferGroup might effectively bypass the decorator implementation

