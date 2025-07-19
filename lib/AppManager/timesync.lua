-- Lua Types for custom TimeSync specific bindings
-- Written by Nate Grift
-- This file should match the bindings in the app_runtime.cpp file

-- Locks LVGL for thread-safe operations
function LVGL_lock()
end

-- UnLocks LVGL for thread-safe operations
function LVGL_unlock()
end

-- Exits the app
function exit()
end

--- Opens an app by name
--- @param name string
function openApp(name)
end

-- Returns the current app name
--- @return string
function currentApp()
end

------------------------------------
-- EVENTS
-- These are events that an app can listen
---------------------------
function OnClose()
end

