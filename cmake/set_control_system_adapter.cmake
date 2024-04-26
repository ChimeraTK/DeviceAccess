#######################################################################################################################
# set_control_system_adapter.cmake
#
# Set the ControlSystemAdapter for a ApplicationCore server
#
# Expects the following input variable:
#   ADAPTER - String specifiing the adapter, either DOOCS, OPCUA or EPICSIOC
#
# This macro will add the selected adapter as a dependency and set the following variables:
#
#   Adapter_LINK_FLAGS - Link flags provided by the adapter library
#   Adapter_LIBRARIES  - Libraries that are dependencies of the adapter
#
#######################################################################################################################

#######################################################################################################################
#
# IMPORTANT NOTE:
#
# DO NOT MODIFY THIS FILE inside a project. Instead update the project-template repository and pull the change from
# there. Make sure to keep the file generic, since it will be used by other projects, too.
#
# If you have modified this file inside a project despite this warning, make sure to cherry-pick all your changes
# into the project-template repository immediately.
#
#######################################################################################################################

# Select control system adapter
if(ADAPTER STREQUAL "OPCUA")
  message("Building against the OPC UA ControlSystemAdater")
  find_package(ChimeraTK-ControlSystemAdapter-OPCUAAdapter 03.00 REQUIRED)
  set_target_properties(ChimeraTK::ChimeraTK-ControlSystemAdapter-OPCUAAdapter PROPERTIES IMPORTED_GLOBAL TRUE)
  add_library(ChimeraTK::SelectedAdapter ALIAS ChimeraTK::ChimeraTK-ControlSystemAdapter-OPCUAAdapter)
elseif(ADAPTER STREQUAL "DOOCS")
  message("Building against the DOOCS ControlSystemAdater")
  find_package(ChimeraTK-ControlSystemAdapter-DoocsAdapter 01.08 REQUIRED)
  set_target_properties(ChimeraTK::ChimeraTK-ControlSystemAdapter-DoocsAdapter PROPERTIES IMPORTED_GLOBAL TRUE)
  add_library(ChimeraTK::SelectedAdapter ALIAS ChimeraTK::ChimeraTK-ControlSystemAdapter-DoocsAdapter)
elseif(ADAPTER STREQUAL "EPICSIOC")
  message("Building against the EPICS IOC ControlSystemAdater")
  find_package(ChimeraTK-ControlSystemAdapter-EPICS-IOC-Adapter 02.01 REQUIRED)
  set_target_properties(ChimeraTK::ChimeraTK-ControlSystemAdapter-EPICS-IOC-Adapter PROPERTIES IMPORTED_GLOBAL TRUE)
  add_library(ChimeraTK::SelectedAdapter ALIAS ChimeraTK::ChimeraTK-ControlSystemAdapter-EPICS-IOC-Adapter)
elseif(ADAPTER STREQUAL "EPICS7IOC")
  message("Building against the EPICS ver. 7.0 IOC ControlSystemAdater")
  find_package(ChimeraTK-ControlSystemAdapter-EPICS7-IOC-Adapter 02.01 REQUIRED)
  set_target_properties(ChimeraTK::ChimeraTK-ControlSystemAdapter-EPICS7-IOC-Adapter PROPERTIES IMPORTED_GLOBAL TRUE)
  add_library(ChimeraTK::SelectedAdapter ALIAS ChimeraTK::ChimeraTK-ControlSystemAdapter-EPICS7-IOC-Adapter)
elseif(ADAPTER STREQUAL "TANGO")
  message("Building against the Tango ControlSystemAdater")
  find_package(ChimeraTK-ControlSystemAdapter-TangoAdapter 01.00 REQUIRED)
  set_target_properties(ChimeraTK::ChimeraTK-ControlSystemAdapter-TangoAdapter PROPERTIES IMPORTED_GLOBAL TRUE)
  add_library(ChimeraTK::SelectedAdapter ALIAS ChimeraTK::ChimeraTK-ControlSystemAdapter-TangoAdapter)
else()
  message(FATAL_ERROR "Please select your ControlSystemAdapter to use by passing to the cmake command line:\n"
                      "   -DADAPTER=DOOCS to build a DOOCS server\n"
                      "   -DADAPTER=OPCUA to build an OPC UA server\n"
                      "   -DADAPTER=EPICSIOC to build an EPICS ver. 3.16 IOC\n"
                      "   -DADAPTER=EPICS7IOC to build an EPICS ver. 7.0 IOC\n"
                      "   -DADAPTER=TANGO to build a Tango device server")
endif()
