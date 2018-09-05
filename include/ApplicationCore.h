/**
 * ApplicationCore.h
 *
 *  Created on: Jun 14, 2016
 *      Author: Martin Hierholzer
 *
 *  This is the main header file for the ApplicationCore library. It brings all includes and functionality needed
 *  for writing an application.
 */

#include <ChimeraTK/Utilities.h>   // for ChimeraTK::setDMapFilePath(), which is used by all applications

#include "Application.h"
#include "ScalarAccessor.h"
#include "ArrayAccessor.h"
#include "ApplicationModule.h"
#include "DeviceModule.h"
#include "ControlSystemModule.h"
#include "VariableGroup.h"
#include "ModuleGroup.h"
#include "VirtualModule.h"
#include "ApplicationException.h"
