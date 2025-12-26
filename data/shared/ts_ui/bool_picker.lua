-- ts_ui.bool_picker
-- Simple On/Off toggle picker

local lvgl = require("lvgl")
local colors = require("ts_ui.colors")

local M = {}

-- Active picker instance
local active_picker = nil

--- Show the boolean picker modal
-- @param options table:
--   - title: string - Title to display
--   - value: boolean - Initial value (true = On, false = Off)
--   - on_save: function(key, type, value) - Called with boolean value
--   - key: string - Setting key to pass to on_save
function M.show(options)
    options = options or {}
    local title = options.title or "Toggle"
    local current_value = options.value or false
    local on_save = options.on_save
    local key = options.key or "boolean"
    
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
            y_ofs = 20,
        },
    }
    
    -- Button references for updating styles
    local on_btn, off_btn
    
    -- Function to update button styles based on value
    local function update_buttons(is_on)
        if is_on then
            on_btn.bg_color = colors.primary
            off_btn.bg_color = colors.card
        else
            on_btn.bg_color = colors.card
            off_btn.bg_color = colors.primary
        end
    end
    
    -- On button
    on_btn = overlay:Object {
        w = 60,
        h = 36,
        bg_color = current_value and colors.primary or colors.card,
        radius = 10,
        outline_width = 0,
        border_width = 0,
        align = {
            type = lvgl.ALIGN.CENTER,
            x_ofs = -38,
            y_ofs = 0,
        },
    }
    on_btn:clear_flag(lvgl.FLAG.SCROLLABLE)
    
    on_btn:Label {
        text = "On",
        text_color = colors.text,
        text_font = lvgl.BUILTIN_FONT.MONTSERRAT_14,
        align = lvgl.ALIGN.CENTER,
    }
    
    on_btn:onClicked(function()
        current_value = true
        update_buttons(true)
    end)
    
    -- Off button
    off_btn = overlay:Object {
        w = 60,
        h = 36,
        bg_color = (not current_value) and colors.primary or colors.card,
        radius = 10,
        outline_width = 0,
        border_width = 0,
        align = {
            type = lvgl.ALIGN.CENTER,
            x_ofs = 38,
            y_ofs = 0,
        },
    }
    off_btn:clear_flag(lvgl.FLAG.SCROLLABLE)
    
    off_btn:Label {
        text = "Off",
        text_color = colors.text,
        text_font = lvgl.BUILTIN_FONT.MONTSERRAT_14,
        align = lvgl.ALIGN.CENTER,
    }
    
    off_btn:onClicked(function()
        current_value = false
        update_buttons(false)
    end)
    
    -- Save button
    local save_btn = overlay:Object {
        w = 140,
        h = 40,
        bg_color = colors.primary,
        radius = 10,
        outline_width = 0,
        border_width = 0,
        align = {
            type = lvgl.ALIGN.BOTTOM_MID,
            x_ofs = 0,
            y_ofs = -25,
        },
    }
    save_btn:clear_flag(lvgl.FLAG.SCROLLABLE)
    
    save_btn:Label {
        text = "Save",
        text_color = colors.text,
        align = lvgl.ALIGN.CENTER,
    }
    
    save_btn:onClicked(function()
        M.hide()
        if on_save then
            on_save(key, "boolean", current_value)
        end
    end)
    
    return overlay
end

--- Hide the boolean picker
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

