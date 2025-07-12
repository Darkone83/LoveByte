#ifndef WEB_MESSAGE_H
#define WEB_MESSAGE_H

#include <ESPAsyncWebServer.h>

// Adds the cloud messenger page at /lb/cloud
void setupMessagePageRoutes(AsyncWebServer& server);

#endif // WEB_MESSAGE_H
