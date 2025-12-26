#ifndef APP_RUNTIME_H
#define APP_RUNTIME_H
#include "AppManager.h"
#include <cstdint>

void runLuaScriptTask(void *pvParameters);
bool appRunning();
void openLuaApp(AppConfig *appConfig);
void closeLuaApp();

// Show a notification via the Lua notification system
// Returns true if the notification was shown, false if Lua is not ready
bool showLuaNotification(int8_t id, const char* title, const char* message, bool important);

#endif // APP_RUNTIME_H
