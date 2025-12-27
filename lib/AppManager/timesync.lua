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

--- Current app info table
--- @class currentApp
currentApp = {}

--- Returns the current app name
--- @return string
function currentApp.name()
end

--- Returns the current app's primary color (e.g., "#E9444C")
--- @return string
function currentApp.primaryColor()
end

------------------------------------
-- NOTIFICATIONS
-- Notification system bindings
------------------------------------

--- Global notifications table
--- @class notifications
notifications = {}

--- Dismiss a notification by ID
--- This stops vibration and sound, and removes the notification from the queue
--- @param id number The notification ID to dismiss
function notifications.dismiss(id)
end

--- Called by C++ to show a notification
--- This function is implemented in ts_ui.notification module
--- @param id number The notification ID
--- @param title string The notification title
--- @param message string The notification message
--- @param important boolean Whether this is an important notification
function _showNotification(id, title, message, important)
end

------------------------------------
-- TIME EVENTS
-- Timer and alarm event management
------------------------------------

--- Global timeEvents table
--- @class timeEvents
timeEvents = {}

--- Event type constants
--- ALARM = 0, TIMER = 1

--- Add a new time event
--- @param type number Event type (0 = ALARM, 1 = TIMER)
--- @param expireTime number Unix timestamp when the event expires
--- @param label string Optional label for the event
--- @param appName string Optional app name associated with the event
--- @return number The generated event ID
function timeEvents.add(type, expireTime, label, appName)
end

--- Delete a time event by ID
--- @param id number The event ID to delete
--- @return boolean True if the event was deleted
function timeEvents.delete(id)
end

--- Get a time event by ID
--- @param id number The event ID
--- @return table|nil The event table or nil if not found
--- Event table: { id, type, expireTime, label, appName }
function timeEvents.getById(id)
end

--- Get all time events of a specific type
--- @param type number Event type (0 = ALARM, 1 = TIMER)
--- @return table Array of event tables
function timeEvents.getAllByType(type)
end

--- Clear all time events of a specific type
--- @param type number Event type (0 = ALARM, 1 = TIMER)
function timeEvents.clearByType(type)
end

--- Get the current Unix timestamp
--- @return number Current Unix timestamp
function timeEvents.now()
end

------------------------------------
-- EVENTS
-- These are events that an app can listen
------------------------------------
function OnClose()
end

