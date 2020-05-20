Technical specification: data validity propagation {#spec_dataValidityPropagation}
==================================================================================

> **DRAFT VERSION, WRITE-UP IN PROGRESS!**

Specification version v1.0

1. General idea
---------------

### 1.1 Version v1.0

* 1.1.1 In ApplicationCore each variable has a data validiy flag attached to it. ChimeraTK::DataValidity can be 'ok' or 'faulty'.
* 1.1.2 This flag is automatically propagated: If one of the inputs of an ApplicationModule is faulty, the data validity of the module becomes faulty, which means
all outputs of this module will automatically be flagged as faulty.
      Fan-outs might be special cases (see 2.4).
* 1.1.3 If a device is in error state, all variables which are read from it shall be marked as 'faulty'. This flag is then propagated through all the modules (via 1.1.2) so it shows up in the control system.
* 1.1.4 The user has the possibility to query the data validity of the module
* 1.1.5 The user has the possibility to set the data validity of the module to 'faulty'. However, the user cannot actively set the module to 'ok' if one of the inputs is 'faulty'.
* \anchor dataValidity_1_1_6 1.1.6 The user can decide to flag individual outputs as bad. However, the user cannot actively set an output to 'ok' if the data validity of the module is 'faulty' (to be more precise: if the validity of the corresponding  DataFaultCounter is faulty). \ref dataValidity_comment_1_1_6 "(*)"
* 1.1.7 The user can get the data validity flag of individual inputs and take special actions.

### Comments

* \anchor dataValidity_comment_1_1_6 \ref dataValidity_1_1_6 "1.1.6": The alternative implementation to add a data validity flag to the write() function is not a good solution because it does not work with writeAll().

2. Technical implementation
---------------------------

### 2.1 MetaDataPropagatingRegisterDecorator (*)

* 2.1.1 Each input and each output of a module (or fan out) is decorated with a ChimeraTK::MetaDataPropagatingRegisterDecorator.
* 2.1.2 The decorator knows about the DataFaultCounter to which it is associated.

* 2.1.3 **read:** For each read operation it checks the incoming data validity and informs the associated ChimeraTK::DataFaultCounter about the status.
* 2.1.5 **write:** When writing, the decorator is checking the validity flags of the DataFaultCounter and the individual flag of the output. Only if both are 'ok' the output validity is 'ok', otherwise the outgoing data is send as 'faulty'.

### 2.2 DataFaultCounter

* 2.2.1 A ChimeraTK::DataFaultCounter is assciated with several inputs and outputs (usually all inputs and outsput of a module, see section 2.3 about ApplicationModules).
* 2.2.2 It has an internal counter variable to keep track how many inputs have reported a data fault.
* 2.2.3 The overall data validity is only 'ok' if the counter is zero (all inputs have validity 'ok), otherwise the overall data validity is 'faulty'.

### 2.3 ApplicationModule

* 2.3.1 Each ApplicationModule has one DataFaultCounter.
* 2.3.2 All inputs and output are connected to it.
* 2.3.3 The main loop of the module usualy does not care about data validity. If any input is invalid, all outputs are automatically invalid. The loop just runs through normaly, even if an inputs has invalid data. (*)
* 2.3.4 Inside the ApplicationModule main loop the module's DataFaultCounter is accessible. The user can call increment and decrement it, but has to be careful to do this in pairs. The more common use case will be to query the module's data validity.

### 2.4 TriggerFanOut

The TriggerFanOut is special in the sense that it does not compute anything, but reads multiple independent poll-type inputs when a trigger arrived, and pushes them out. In contrast to an ApplicationModule or one of the data fan-outs, their data validities are not connected.

* 2.4.1 The TriggerFanOut instantiates one DataFaultCounter for each input variable.
* 2.4.2 The connection code associates the input and all corresponding outputs to the correct DataFaultCounter.
* \anchor dataValidity_2_4_3 2.4.3 Although the trigger conceptually has data type 'void', it can also be `faulty` \ref dataValidity_comment_2_4_3 "(*)". An invalid trigger will result be processed, but all data that is being read out is flagged as `faulty`.

### 2.5 Interaction with exception handling

See @ref spec_execptionHandling.

* 2.5.1 When creating the objects, ApplicationCore takes care that the MetaDataPropagatingRegisterDecorator is always placed around the ExceptionHandlingDecorators. Like this
  a `faulty` flag raised by the ExceptionHandlingDecorator is automatically picked up by the MetaDataPropagatingRegisterDecorator.
* 2.5.2 The first failing read returns with the old data and the 'faulty' flag. Like this the flag is propagated to the outputs. Only further reads might freeze until the device is available again.

### Comments:

* to 2.1 The MetaDataPropagatingRegisterDecorator also propagates the version number, not only the data validity flag. Hence it's not called DataValidityPropagatingRegisterDecorator.
* to 2.3.3 If there would be some outputs which are still valid if a particular input is faulty it means that they are not connected to that input. This usualy is an indicator that the module is doing unrelated things and should be split.
* to 2.3.3 A change of the data validity in the DataFaultCounter does not automatically change the validity on all outputs. A module might not always write all of its outputs. If the DataFaultCounter reports 'faulty', those outputs which are not written stay valid. This is correct because their calculation was not affected by the faulty data yet. And if an output which has faulty data is not updated when the data validity goes back to 'ok' it also stays 'faulty', which is correct as well.

* \anchor dataValidity_comment_2_4_3 \ref dataValidity_2_4_3 "to 2.4.3"  A void variable can be invalid if the sending device fails, hence there is no new data, and then a heartbeat times out and raises an exception. This will result in a void variable with data validity `faulty` because it does not originate form a recevied message.



3. Implementation details
-------------------------

* 3.1 The DataFaultCounter is a helper class around the counter variable. It facilitates instantiation and evaluation of the counter.
* 3.2 The decorators which manipulate the data fault counter are responsible for counting up and down in pairs, such that the counter goes back to 0 if all data is ok, and never becomes negative.
* 3.3 To 2.1.2 and 2.2.1: Associated means that the MetaDataPropagatingRegisterDecorator gets a pointer to the DataFaultCounter. It does not own it. As the calling code of the decorator is executed in the main loop of the module or fan-out which owns the DataFaultCounter, it should be safe to shut down the application.
* 3.4 Interface of the DataFaultCounter (implements 2.2)
    * 3.4.1 The counter variable itself is protected. It is increased and decreased through increment() and decrement() functions.
    * 3.4.2 The decrement function has an assertions that the counter does not become negative (or "underflow" because it probably is unsigned). This simplifies debugging in case the calling code does not execute the increment and decrement in pairs.
    * 3.4.3 A conveninece function getDataValidity() checks whether the counter is 0 and returns 'ok' or 'faulty' accordingly. This is just to avoid the if-statement and picking the right return value in all using code.
    * 3.4.4 When instantiated, the counter starts at 0. If associated objects are faulty at start it is their responsibility to increment the counter.

4. Known issues
---------------

* 4.1 There is no DataFaultCounter object (2.2) at the moment. The ApplicationModule itself holds the counter variable and implements the increase, decrease and getDataValidity functions. This prevents 2.4 from being implemented properly.
* 4.2 The ThreadedFanOut does not propagate the data validity
* 4.3 The ConsumingFanOut does not propagate the data validity
* 4.4 The TriggerFanOut does not propagate data validity (not even the way an ApplicationModule does)
