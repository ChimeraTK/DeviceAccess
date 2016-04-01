#include <DoocsAdapter.h>

// Include all the control applications you want in this server
#include "IndependentControlCore.h"

BEGIN_DOOCS_SERVER("Cosade server", 10)
   // Create static instances for all applications cores. They must not have overlapping
   // process variable names ("location/protery" must be unique).
   static IndependentControlCore independentControlCore(doocsAdapter.getDevicePVManager());
END_DOOCS_SERVER()
