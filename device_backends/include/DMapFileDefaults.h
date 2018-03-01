#ifndef CHIMERA_TK_DMAP_FILE_DEFAULTS_H
#define CHIMERA_TK_DMAP_FILE_DEFAULTS_H

// This header should only be included in implementations because it contains
// the DMAP_FILE_PREFIX constant which is defined as compile paramter by CMAKE. You don't
// want to expose this detail to user code.

namespace ChimeraTK {
  static const std::string DMAP_FILE_ENVIROMENT_VARIABLE("DMAP_FILE"); ///< The envorinment variable which contains the dmap file
  static const std::string DMAP_FILE_DEFAULT_DIRECTORY(std::string(DMAP_FILE_PREFIX)
                                                       +"/etc/ChimeraTK/"); ///< The default dmap file directorY (without file name)
  static const std::string DMAP_FILE_DEFAULT_NAME("devices.dmap"); ///< The default dmap file name (without directory)
  
}// namespace ChimeraTK

#endif //CHIMERA_TK_DMAP_FILE_DEFAULTS_H
