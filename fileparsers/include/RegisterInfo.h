/*
 * RegisterInfo.h
 *
 *  Created on: Mar 1, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_REGISTER_INFO_H
#define MTCA4U_REGISTER_INFO_H

#include "RegisterPath.h"
#include "RegisterPlugin.h"

namespace mtca4u {

  // forward declarations
  template<typename UserType>
  class BufferingRegisterAccessorImpl;

  class RegisterAccessor;

  /** DeviceBackend-independent register description. */
  class RegisterInfo {

    public:

      /** Virtual destructor */
      virtual ~RegisterInfo() {}

      /** Return full path name of the register (including modules) */
      virtual RegisterPath getRegisterName() const = 0;

      /** Return number of elements in register */
      virtual unsigned int getNumberOfElements() const = 0;

      /** Obtain a potentially modified buffering register accessor from the given accessor. Any plugins specified
       *  in the map for this register might modify the accessor. */
      template<typename UserType>
      boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > getBufferingRegisterAccessor(
          boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > accessor) const {
        for(auto i = pluginList.begin(); i != pluginList.end(); ++i) {
          accessor = (*i)->getBufferingRegisterAccessor<UserType>(accessor);
        }
        return accessor;
      }

      /** Obtain a potentially modified (non-buffering) register accessor from the given accessor. Any plugins
       *  specified in the map for this register might modify the accessor. */
      boost::shared_ptr<RegisterAccessor> getRegisterAccessor(boost::shared_ptr<RegisterAccessor> accessor) const {
        for(auto i = pluginList.begin(); i != pluginList.end(); ++i) {
          accessor = (*i)->getRegisterAccessor(accessor);
        }
        return accessor;
      }

    protected:

      /** list of plugins */
      std::vector< boost::shared_ptr<RegisterPlugin> > pluginList;

  };

} /* namespace mtca4u */

#endif /* MTCA4U_REGISTER_INFO_H */
