Technical specification: StatusAggregator {#spec_StatusAggregator}
====================================================================

> **DRAFT VERSION, WRITE-UP IN PROGRESS!**

> **NOTE: As aggregator and monitor are tighly coupled, we might have one spec
> page for both **


## Introduction ##

The `ChimeraTK::StatusAggregator` provides a facitlity to collect multiple
statuses and merge them into a single output status. The sources of input statuses can be:

* Instances of `ChimeraTK::StatusMonitor` 
* Other instances of the StatusAggregator

## High-level requirements ##

### R1.1 Automatic detection of instances ###

The StatusMonitor and StatusAggregator instances to be connected into the
StatusAggregator should be found automatically as follows: 

* **R1.1.1**: A StatusAggregator aggregates all instances in the hierarchy of
  ApplicationCore Modules which are at the same hierarchy level or below
* **R1.1.2**: Two StatusAggregator instances cannot be on the same hierarchy
  level - this should throw a `ChimeraTK::logic_error`
* **R1.1.3**: If a StatusAggregator is found at some hierarchy level below, the
  search is stopped for that branch of the hierarchy tree, i.e. all variables
  which are already aggregated by that aggregator are not aggregated again
* **R1.1.4**: The StatusAggregator provides a `disable` input, which
  deactivates the aggregator and sets the output status to OFF.

### R1.2 Priorization of status values ###

The StatusAggregator is configurable (by a constructor parameter, not a PV) in
the way how the statuses are prioritised. The status with the highest priority
within the aggregated instances defines the aggregated status. The following
options exist (highest priority first): 

1. error - warning - off - ok
2. error - warning - ok - off
3. error - warning - ok or off - mixed state of ok/off results in a warning
4. off - error - warning - ok

### Constraints and issues ###

* As detection of underlying instances can only be performed in the
  constructor, the StatusAggregator has to be declared in user code after all
  instances of `ChimeraTK::StatusMonitor` in a Module.
* Usage of `HierarchyModifier::moveToRoot` on a StatusAggregator is
  controversial:
  * Either: Detection of aggregated instances may take place from the virtual
    hierarchy level of the original (C++) parent module downwards. The
    Aggregator and its status output would then appear on the root level.
  * Or: `HierarchyModifier::moveToRoot` is not allowd on StatusAggregators

## Implementation ##

### Requirements ###

* **R2.1**: The detection of instances needs to be performed recursively from the StatusAggregator's virtual parent module on
* **R2.2**: The detection has to work on instances the are modified by any of
  the `ChimeraTK::HierarchyModifier`s 

### Constraints and issues ###

* The detection and connection of the instances to be aggregated has to be
  performed in the constructor, later the variable household is fixed and the
  status inputs can not be added anymore.

* As a consequence of the implementation requirements, in order to get the
  virtual parent module, the entire Application needs to be virtualised and the
  search algorithm must then navigate to the parent module.

* Related to R1.1.3: If a StatusAggregator is found on a level below, the
  instances of `ChimeraTK::StatusMonitor`s on that level must be ignored. 
