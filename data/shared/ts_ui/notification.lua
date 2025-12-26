-- ts_ui.notification
-- Full-screen notification modal for TimeSync
-- This module is called from C++ when notifications need to be displayed

local lvgl = require("lvgl")
local colors = require("ts_ui.colors")
local icons = require("ts_ui.icons")

local M = {}

-- Active notification modal
local active_notification = nil

--- Show a notification modal
-- @param options table:
--   - id: number - Notification ID
--   - title: string - Notification title
--   - message: string - Notification message
--   - important: boolean - Whether this is an important notification
function M.show(options)
    options = options or {}
    local id = options.id or 0
    local title = options.title or "Notification"
    local message = options.message or ""
    local important = options.important or false
    
    -- Close any existing notification (we already hold the lock)
    if active_notification then
        active_notification.overlay:delete()
        active_notification = nil
    end
    
    -- Full screen overlay
    local overlay = lvgl.Object({
        w = lvgl.HOR_RES(),
        h = lvgl.VER_RES(),
        x = 0,
        y = 0,
        bg_color = colors.background,
        bg_opa = lvgl.OPA(100),
        pad_all = 20,
        outline_width = 0,
        border_width = 0,
        scrollbar_mode = lvgl.SCROLLBAR_MODE.OFF,
    })
    
    active_notification = {
        overlay = overlay,
        id = id,
    }
    
    -- Bell icon at top
    overlay:Label {
        text = icons.bell,
        text_color = colors.primary,
        text_font = lvgl.BUILTIN_FONT.MONTSERRAT_24,
        align = {
            type = lvgl.ALIGN.TOP_MID,
            x_ofs = 0,
            y_ofs = 25,
        },
    }
    
    -- Title (red color for urgency)
    overlay:Label {
        text = title,
        text_color = colors.primary,
        text_font = lvgl.BUILTIN_FONT.MONTSERRAT_16,
        align = {
            type = lvgl.ALIGN.TOP_MID,
            x_ofs = 0,
            y_ofs = 65,
        },
    }
    
    -- Message (centered)
    local msg_label = overlay:Label {
        text = message,
        text_color = colors.text,
        text_font = lvgl.BUILTIN_FONT.MONTSERRAT_14,
        align = {
            type = lvgl.ALIGN.CENTER,
            x_ofs = 0,
            y_ofs = 0,
        },
    }
    
    -- Dismiss button (primary color, centered at bottom)
    local dismiss_btn = overlay:Object {
        w = 140,
        h = 50,
        bg_color = colors.primary,
        radius = 25,
        outline_width = 0,
        border_width = 0,
        align = {
            type = lvgl.ALIGN.BOTTOM_MID,
            x_ofs = 0,
            y_ofs = -30,
        },
    }
    dismiss_btn:clear_flag(lvgl.FLAG.SCROLLABLE)
    
    dismiss_btn:Label {
        text = "Dismiss",
        text_color = colors.text,
        text_font = lvgl.BUILTIN_FONT.MONTSERRAT_14,
        align = lvgl.ALIGN.CENTER,
    }
    
    dismiss_btn:onClicked(function()
        M.dismiss()
    end)
    
    return overlay
end

--- Dismiss the current notification and notify C++
-- NOTE: This is called from onClicked callback which runs inside lv_timer_handler()
-- The mutex is already held by the rendering task, so we do NOT lock here
function M.dismiss()
    if active_notification then
        local id = active_notification.id
        
        -- Delete the overlay (we're inside a callback, mutex already held)
        active_notification.overlay:delete()
        active_notification = nil
        
        -- Notify C++ to dismiss the notification (stops sound/vibration)
        -- This may trigger showing the next notification in queue
        if notifications and notifications.dismiss then
            notifications.dismiss(id)
        end
    end
end

--- Hide the notification modal without notifying C++
-- NOTE: Called from show() which holds the lock, so we do NOT lock here
function M.hide()
    if active_notification then
        active_notification.overlay:delete()
        active_notification = nil
    end
end

--- Check if notification is visible
function M.is_visible()
    return active_notification ~= nil
end

--- Get the current notification ID
function M.get_current_id()
    if active_notification then
        return active_notification.id
    end
    return nil
end

-- Global function that C++ will call to show notifications
-- This is registered as a global so C++ can easily call it
function _showNotification(id, title, message, important)
    M.show({
        id = id,
        title = title,
        message = message,
        important = important or false,
    })
end

-- Register the global function
_G._showNotification = _showNotification

return M

