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

### R1.2 Priorization of status values ###

The StatusAggregator is configurable (by a constructor parameter, not a PV) in
the way how the statuses are prioritised. The status with the highest priority
within the aggregated instances defines the aggregated status. The following
options exist (highest priority first): 

1. error - warning - off - ok
2. error - warning - ok - off
3. error - warning - ok or off - mixed state of ok/off results in a warning
4. off - error - warning - ok

## Implementation ##

### Requirements ###

* **R2.1**: The instances need to be aggregated on the `ChimeraTK::VirtualModule` on the aggregator's level

### Constraints and issues ###

* The detection and connection of the instances to be aggregated has to be
  performed in the constructor, later the variable household is fixed and the
  status inputs can not be added anymore 



## Questions & issues ##

* Which HierarchyModifiers should be supported? This questions also holds for
  the StatusMonitor. 
  * Which Modifiers exactly cause problems? Probably these are are  bugs and we
    need separate treatment. But we should clarify if this blocks required use
    cases of the aggregator.
* Do we want a disable for the StatusAggregator, like there is for the
  monitors?

