Technical specification: TransferElement {#spec_TransferElement}
========================================

> **DRAFT VERSION, WRITE-UP IN PROGRESS!**

This currently is just a stub with detail snippets.

## Introduction ##

## Unsorted collection of details ###

* preRead and postRead must always be called in pairs, as well as preWrite and postWrite. It might be that the user buffer has to be swapped out during the transfer (while taking away the ownership of the calling code), and this must be restored in the postXxx action. If it is not called you might end up with an invalid accessors

### "Decorators" and similar functionality ###

* doPostRead() and doPostWrite()
  When decorating doPostRead() and doPostWrite() the delegating code
  must always call postRead() and postWrite() of the target (with out 'do'). This takes care that in transfer groups the actual doXXX action is only performed once per underlying hardware accessing element.
* doXxxTransferYyy()
  When decorating the hardware accessing transfers do **not** call xxxTransferYyy (in contrast to the pre/post operations). The xxxTransferYyy() functions (without 'do') only catch exceptions, and this should not happen on each level.  
