#ifndef WEB_FILEMAN_H
#define WEB_FILEMAN_H

#include <ESPAsyncWebServer.h>

// Call this in your setup() after SD and server are initialized.
void setupFileManagerRoutes(AsyncWebServer& server);

#endif
