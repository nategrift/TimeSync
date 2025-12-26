-- ts_ui.roller_picker
-- Simple roller/dropdown picker for string values

local lvgl = require("lvgl")
local colors = require("ts_ui.colors")

local M = {}

-- Active picker instance
local active_picker = nil

-- Size presets (width in pixels)
local SIZES = {
    [1] = 50,   -- Small: 2 numbers/chars
    [2] = 80,   -- Medium: ~4 letters
    [3] = 120,  -- Large: 5-7 letters
}

--- Show the roller picker modal
-- @param options table:
--   - title: string - Title to display
--   - values: table - Array of string options {"option1", "option2", ...}
--   - selected: string or number - Initial selected value or index
--   - size: number - Width preset (1=small, 2=medium, 3=large)
--   - on_save: function(key, type, value) - Called with selected value
--   - key: string - Setting key to pass to on_save
function M.show(options)
    options = options or {}
    local title = options.title or "Select"
    local values = options.values or {}
    local size = options.size or 2
    local on_save = options.on_save
    local key = options.key or "roller"
    
    -- Close any existing picker
    if active_picker then
        M.hide()
    end
    
    -- Build options string for roller
    local options_str = table.concat(values, "\n")
    
    -- Find initial selected index
    local init_index = 0
    if type(options.selected) == "number" then
        init_index = options.selected - 1  -- Convert to 0-based
    elseif type(options.selected) == "string" then
        for i, v in ipairs(values) do
            if v == options.selected then
                init_index = i - 1
                break
            end
        end
    end
    
    -- Get width from size preset
    local roller_width = SIZES[size] or SIZES[2]
    
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
            y_ofs = 10,
        },
    }
    
    -- Roller (centered)
    local roller = overlay:Roller {
        options = { options = options_str, mode = 0 },
        w = roller_width,
        h = 40,
        bg_color = colors.card,
        border_width = 0,
        outline_width = 0,
        radius = 8,
        align = lvgl.ALIGN.CENTER,
    }
    roller:set { selected = { selected = init_index, anim = 0 } }
    roller:set_style({ bg_opa = lvgl.OPA(0) }, lvgl.PART.SELECTED)
    
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
            y_ofs = -20,
        },
    }
    save_btn:clear_flag(lvgl.FLAG.SCROLLABLE)
    
    save_btn:Label {
        text = "Set",
        text_color = colors.text,
        align = lvgl.ALIGN.CENTER,
    }
    
    save_btn:onClicked(function()
        -- Get selected value string
        local selected_str = roller:get_selected_str()
        
        M.hide()
        
        if on_save then
            on_save(key, "roller", selected_str)
        end
    end)
    
    return overlay
end

--- Hide the roller picker
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

