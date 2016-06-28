/*
 * Module.cc
 *
 *  Created on: Jun 27, 2016
 *      Author: Martin Hierholzer
 */

#include "Application.h"
#include "Module.h"

namespace ChimeraTK {

  Module::Module() {
    assert(&(Application::getInstance()) != nullptr);
    Application::getInstance().registerModule(*this);
  }

} /* namespace ChimeraTK */
