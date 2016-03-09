/*
 * RegisterCatalogue.h
 *
 *  Created on: Mar 8, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_REGISTER_CATALOGUE_H
#define MTCA4U_REGISTER_CATALOGUE_H

#include <map>

#include "RegisterInfo.h"
#include "RegisterPath.h"

namespace mtca4u {

  /** Catalogue of register information */
  class RegisterCatalogue {
  
    public:
    
      /** Get register information for a given full path name. */
      boost::shared_ptr<RegisterInfo> getRegister(const RegisterPath& registerPathName);
    
      /** Add register information to the catalogue. The full path name of the register is taken from the
       *  RegisterInfo structure. */
      void addRegister(boost::shared_ptr<RegisterInfo> registerInfo);
  
    protected:
    
      /** Map of register path names to register information. A more efficient way to store this information
       *  would be a tree-like structure, but this can be optimised later without an interface change. */
      std::map< RegisterPath, boost::shared_ptr<RegisterInfo> > catalogue;
  
  };


} /* namespace mtca4u */

#endif /* MTCA4U_REGISTER_CATALOGUE_H */
