#ifndef GRAPHICS_DRIVER_H
#define GRAPHICS_DRIVER_H

#include "IUIComponent.h"
#include <string>
#include <map>
#include <memory>
#include <vector>
#include "lvgl.h"

extern "C" {
#include "display.h"
#include "gc9a01.h"
}



class GraphicsDriver {
private:
    static void lvgl_task(void *arg);

public:
    GraphicsDriver();
    void init();
    void addTextToCenter(const char *text);
};

#endif // GRAPHICS_DRIVER_H
