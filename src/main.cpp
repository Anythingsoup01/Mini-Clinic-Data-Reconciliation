#include "webserver.h"
#include "routes/routes.h"

int main(void) {
  Webserver server;

  //
  //  GET
  //

  server.HandleRoute(_Method::GET, "/api/home", HandleHome);
  server.HandleRoute(_Method::GET, "/api/styles.css", HandleHomeStyles);
  server.HandleRoute(_Method::GET, "/api/scripts.js", HandleHomeScripts);

  server.HandleRoute(_Method::GET, "/api/login", HandleLogin);
  
  //
  //  POST
  //

  server.HandleRoute(_Method::POST, "/api/auth/login", HandleLoginLogic);
  server.HandleRoute(_Method::POST, "/api/reconcile/medication", HandleReconcileMedication);
  server.HandleRoute(_Method::POST, "/api/validate/data_quality", HandleValidateDataQuality);

  server.Run();
 
  return 0;

}
