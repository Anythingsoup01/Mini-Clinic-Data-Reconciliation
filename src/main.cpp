
#include "webserver.h"
#include "llm_api.h"
#include "routes/routes.h"
#include "serializer.h"

#include <iostream>

int main(void) {
  _Config config = LoadConfig();
  if (config.ApiKey.empty()) {
    return -1;
  }

  return 0;

  LlmAPI::Init(config.ApiKey);

  Webserver server(config.Port);

  server.HandleRoute(_Method::GET, "/api/home", HandleRoot);
  server.HandleRoute(_Method::GET, "/api/login", HandleLogin);

  server.HandleRoute(_Method::POST, "/api/auth/login", HandleLoginLogic);
  server.HandleRoute(_Method::POST, "/api/reconcile/medication", HandleReconcileMedication);
  server.HandleRoute(_Method::POST, "/api/validate/data_quality", HandleValidateDataQuality);

  server.Run();
 
  LlmAPI::Shutdown();
  return 0;

}
