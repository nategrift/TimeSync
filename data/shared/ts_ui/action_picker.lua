-- ts_ui.action_picker
-- Confirmation screen for actions (like restart)

local lvgl = require("lvgl")
local colors = require("ts_ui.colors")

local M = {}

-- Active picker instance
local active_picker = nil

--- Show the action picker modal
-- @param options table:
--   - title: string - Title to display
--   - message: string - Optional message/description
--   - confirm_text: string - Text for confirm button (default "Confirm")
--   - on_confirm: function() - Called when confirmed
--   - key: string - Setting key (for consistency)
function M.show(options)
    options = options or {}
    local title = options.title or "Confirm"
    local message = options.message or ""
    local confirm_text = options.confirm_text or "Confirm"
    local on_confirm = options.on_confirm
    
    -- Close any existing picker
    if active_picker then
        M.hide()
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
    
    active_picker = {
        overlay = overlay,
    }
    
    -- Title
    overlay:Label {
        text = title,
        text_color = colors.text,
        text_font = lvgl.BUILTIN_FONT.MONTSERRAT_16,
        align = {
            type = lvgl.ALIGN.TOP_MID,
            x_ofs = 0,
            y_ofs = 30,
        },
    }
    
    -- Message (if provided)
    if message ~= "" then
        overlay:Label {
            text = message,
            text_color = colors.secondary_text,
            text_font = lvgl.BUILTIN_FONT.MONTSERRAT_12,
            align = {
                type = lvgl.ALIGN.CENTER,
                x_ofs = 0,
                y_ofs = -20,
            },
        }
    end
    
    -- Confirm button (centered, primary color)
    local confirm_btn = overlay:Object {
        w = 120,
        h = 50,
        bg_color = colors.primary,
        radius = 12,
        outline_width = 0,
        border_width = 0,
        align = {
            type = lvgl.ALIGN.CENTER,
            x_ofs = 0,
            y_ofs = 20,
        },
    }
    confirm_btn:clear_flag(lvgl.FLAG.SCROLLABLE)
    
    confirm_btn:Label {
        text = confirm_text,
        text_color = colors.text,
        text_font = lvgl.BUILTIN_FONT.MONTSERRAT_14,
        align = lvgl.ALIGN.CENTER,
    }
    
    confirm_btn:onClicked(function()
        M.hide()
        if on_confirm then
            on_confirm()
        end
    end)
    
    -- Cancel button (bottom, card color)
    local cancel_btn = overlay:Object {
        w = 100,
        h = 36,
        bg_color = colors.card,
        radius = 10,
        outline_width = 0,
        border_width = 0,
        align = {
            type = lvgl.ALIGN.BOTTOM_MID,
            x_ofs = 0,
            y_ofs = -25,
        },
    }
    cancel_btn:clear_flag(lvgl.FLAG.SCROLLABLE)
    
    cancel_btn:Label {
        text = "Cancel",
        text_color = colors.text,
        text_font = lvgl.BUILTIN_FONT.MONTSERRAT_12,
        align = lvgl.ALIGN.CENTER,
    }
    
    cancel_btn:onClicked(function()
        M.hide()
    end)
    
    return overlay
end

--- Hide the action picker
function M.hide()
    if active_picker then
        active_picker.overlay:delete()
        active_picker = nil
    end
end

--- Check if picker is visible
function M.is_visible()
    return active_picker ~= nil
end

return M


