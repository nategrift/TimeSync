-- ts_ui - TimeSync UI Library
-- Shared UI components and utilities for TimeSync apps
--
-- Usage:
--   local ts_ui = require("ts_ui")
--   ts_ui.app_selector.show()
--   ts_ui.keyboard.show({ title = "Enter text" })

local M = {}

-- Lazy-load sub-modules to save memory when not needed
local function lazy_require(module_name)
    local loaded = nil
    return setmetatable({}, {
        __index = function(_, key)
            if not loaded then
                loaded = require("ts_ui." .. module_name)
            end
            return loaded[key]
        end,
        __call = function(_, ...)
            if not loaded then
                loaded = require("ts_ui." .. module_name)
            end
            if type(loaded) == "function" then
                return loaded(...)
            end
            error("Module ts_ui." .. module_name .. " is not callable")
        end
    })
end

-- Sub-modules
M.app_selector = lazy_require("app_selector")
M.keyboard = lazy_require("keyboard")
M.colors = lazy_require("colors")
M.icons = lazy_require("icons")
M.time_picker = lazy_require("time_picker")
M.date_picker = lazy_require("date_picker")
M.text_input = lazy_require("text_input")
M.roller_picker = lazy_require("roller_picker")
M.bool_picker = lazy_require("bool_picker")
M.action_picker = lazy_require("action_picker")
M.notification = lazy_require("notification")

return M

