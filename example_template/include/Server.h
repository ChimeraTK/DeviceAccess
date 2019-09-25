/*
 * piezoctrlserver.h
 *
 *  Created on: Mar 12, 2018
 *      Author: Christoph Kampmeyer
 */

#ifndef INCLUDE_SERVER_H_
#define INCLUDE_SERVER_H_

#include <ChimeraTK/ApplicationCore/ApplicationCore.h>
// TODO Note to optionally use to trigger
//#include <ChimeraTK/ApplicationCore/PeriodicTrigger.h>


namespace ctk = ChimeraTK;

struct Server : public ctk::Application {

  Server() : Application("ApplicationCore-TemplateServer") {}
  ~Server() override { shutdown(); }

  //ctk::PeriodicTrigger timer{this, "Timer", "Periodic timer for device readout", 1000};

  ctk::ControlSystemModule cs;
  ctk::DeviceModule dev{this, "Device"};
//  ctk::DeviceModule externalTrigger{this, "ExtTrigger"};


  void defineConnections() override;

};


#endif /* INCLUDE_SERVER_H_ */
