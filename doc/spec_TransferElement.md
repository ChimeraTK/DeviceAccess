Technical specification: TransferElement {#spec_TransferElement}
========================================

> **DRAFT VERSION, WRITE-UP IN PROGRESS!**

This currently is just a stub with detail snippets.

## Introduction ##

## Unsorted collection of details ###

* preXxx and posXxx are called always in pairs. Reason: It might be that the user buffer has to be swapped out during the transfer (while taking away the ownership of the calling code), and this must be restored in the postXxx action.
* This holds even if exceptions (both logic_error and runtime_error) are thrown.
* If in preXxx an exception is thrown, the corresponding xxxTransferYyy is not called, instead directly postXxx is called.

Requirement for backend/decoractor implementations:

* In the constructor, *only* ChimeraTK::logic_errror must be thrown. No device communication is allowed at that point, as the device may even be closed.
* In doPreXxx, *only* ChimeraTK::logic_errror must be thrown. No device communication is allowed at this point.
* In doXxxTransferYyy, *only* ChimeraTK::runtime_error must be thrown
* In doPostXxx, *only* ChimeraTK::runtime_error must be thrown. No exception must be thrown if doXxxTransferYyy has not been called (because doPreXxx has thrown a ChimeraTK::logic_error). No device communication is allowed at this point, hence only delayed execptions from the transfer may be thrown.


### Decorators including decorator-like implementations ###

* All functions doPreXxx, doXxxTransferYyy and doPostXxx must delegate to their non-do counterparts (preXxx, xxxTransferYyy and postXxx). Never delegate to the do... of the target implementation functions directly.
* If a function of the own implementation should be called, e.g. if doReadTransferLatest should redirect to doReadTransfer, call to do-version of the function. This is merely to avoid code duplication.
* Decorators must merely delegate doXxxTransferYyy, never add any functionalty there. Reason: TransferGroup might effectively bypass the decorator implementation

