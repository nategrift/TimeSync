#ifndef APP_RUNTIME_H
#define APP_RUNTIME_H
#include "AppManager.h"

void runLuaScriptTask(void *pvParameters);
bool appRunning();
void openLuaApp(AppConfig *appConfig);
void closeLuaApp();

#endif // APP_RUNTIME_H
