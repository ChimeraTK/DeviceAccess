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
#include "ForwardDeclarations.h"

namespace mtca4u {

  /** DeviceBackend-independent register description. */
  class RegisterInfo {

    public:

      /** Virtual destructor */
      virtual ~RegisterInfo() {}

      /** Return full path name of the register (including modules) */
      virtual RegisterPath getRegisterName() const = 0;

      /** Return number of elements in register */
      virtual unsigned int getNumberOfElements() const = 0;
      
      /** Iterators for the list of plugins */
      class plugin_iterator {
        public:
          plugin_iterator& operator++() {    // ++it
            ++iterator;
            return *this;
          }
          plugin_iterator operator++(int) { // it++
            plugin_iterator temp(*this);
            ++iterator;
            return temp;
          }
          const RegisterPlugin& operator*() const {
            return *(iterator->get());
          }
          const RegisterPlugin* operator->() const {
            return iterator->get();
          }
          bool operator==(const plugin_iterator &rightHandSide) {
            return rightHandSide.iterator == iterator;
          }
          bool operator!=(const plugin_iterator &rightHandSide) {
            return rightHandSide.iterator != iterator;
          }
        protected:
          std::vector< boost::shared_ptr<RegisterPlugin> >::const_iterator iterator;
          friend class RegisterInfo;
      };
      
      /** Return iterators for the list of plugins */
      plugin_iterator plugins_begin() const {
        plugin_iterator i;
        i.iterator = pluginList.cbegin();
        return i;
      }
      plugin_iterator plugins_end() const {
        plugin_iterator i;
        i.iterator = pluginList.cend();
        return i;
      }

    protected:

      /** list of plugins */
      std::vector< boost::shared_ptr<RegisterPlugin> > pluginList;

  };

} /* namespace mtca4u */

#endif /* MTCA4U_REGISTER_INFO_H */
