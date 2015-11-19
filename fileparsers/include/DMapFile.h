#ifndef _MTCA4U_DMAPFILE_H_
#define _MTCA4U_DMAPFILE_H_

#warning Using DMapFile is deprecated. The class has been renamed to DeviceInfoMap.

#include "DeviceInfoMap.h"

namespace mtca4u{

  /** Using DMapFile is deprecated. The class has been renamed to DeviceInfoMap.
   *  @deprecated Using DMapFile is deprecated. The class has been renamed to DeviceInfoMap.
   */
 class DMapFile: public DeviceInfoMap{
 public:
   /** The name DRegisterInfo was a misnomer due to refactoring. Use DeviceInfo instead.
    *  This alias does not make DRegisterInfo work the way it did because the members have also been renamed.
    * @deprecated The name DRegisterInfo was a misnomer due to refactoring. Use DeviceInfo instead.
    */
   typedef DeviceInfo DRegisterInfo;
 };

}//namespace mtca4u

#endif /* _MTCA4U_DMAPFILE_H_ */
