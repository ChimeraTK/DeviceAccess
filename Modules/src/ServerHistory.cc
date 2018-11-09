#include "ServerHistory.h"

using namespace ChimeraTK::history;

void ServerHistory::connctAll(ControlSystemModule *cs, std::string subfolder){
  v_float.findTag("CS").connectTo((*cs)[subfolder]);
}



