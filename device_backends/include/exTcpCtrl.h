/*
 * exTcpCtrl.h
 *
 *  Created on: May 29, 2015
 *      Author: adagio
 */

#ifndef EXTCPCTRL_H_
#define EXTCPCTRL_H_

#include <mtca4u/Exception.h>
#include <string>

namespace mtca4u {

///Provides class for exceptions related to RebotDevice
class RebotBackendException : public Exception {
public:
  enum { EX_OPENSOCK, EX_CONNECT, EX_CLOSESOCK,  EX_WRSOCK, EX_RDSOCK, EX_DEVICE_CLOSED, EX_SETIP, EX_SETPORT, EX_SIZEMULT};
  RebotBackendException(const std::string &_exMessage, unsigned int _exID);
};

} //namespace mtca4u

#endif /* EXTCPCTRL_H_ */
