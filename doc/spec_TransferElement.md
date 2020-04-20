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

* In the constructor, *only* ChimeraTK::logic_errror must be thrown.
* In doPreXxx, *only* ChimeraTK::logic_errror must be thrown. No device communication is allowed at this point.
* In doXxxTransferYyy, *only* ChimeraTK::runtime_error must be thrown
* In doPostXxx, *only* exceptions that were risen in doPreXxx or doXxxTransferYyy may be rethrown. This is done by the TransferElement base class in postXxx, so implementations should never actively throw in doPostXxx (but decorators must expect exceptions to be thrown by delegated calls).

### Decorators including decorator-like implementations ###

* All functions doPreXxx, doXxxTransferYyy and doPostXxx must delegate to their non-do counterparts (preXxx, xxxTransferYyy and postXxx). Never delegate to the do... of the target implementation functions directly.
* If a function of the same instance should be called, e.g. if doReadTransferLatest should redirect to doReadTransfer or if doPostRead should call doPostRead of a base class, call to do-version of the function. This is merely to avoid code duplication, hence the surrounding logic of the non-do function is not wanted here.
* Decorators must merely delegate doXxxTransferYyy, never add any functionalty there. Reason: TransferGroup might effectively bypass the decorator implementation

