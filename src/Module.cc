/*
 * Module.cc
 *
 *  Created on: Jun 27, 2016
 *      Author: Martin Hierholzer
 */

#include "Application.h"
#include "Module.h"

namespace ChimeraTK {

  Module::Module(EntityOwner *owner, const std::string &name)
  : EntityOwner(owner, name)
  {
    Application *theApp = &(Application::getInstance());
    assert(theApp != nullptr);
    theApp->overallRegisterModule(*this);
  }

} /* namespace ChimeraTK */
