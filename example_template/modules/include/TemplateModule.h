#ifndef TEMPLATEMODULE_H
#define TEMPLATEMODULE_H

#include <ChimeraTK/ApplicationCore/ApplicationCore.h>


namespace ctk = ChimeraTK;

struct TemplateModule : public ctk::ApplicationModule {

  using ctk::ApplicationModule::ApplicationModule;
  ~TemplateModule(){}

  void mainLoop() override;
};

#endif // TEMPLATEMODULE_H
