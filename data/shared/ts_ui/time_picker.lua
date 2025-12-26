-- ts_ui.time_picker
-- Time picker modal component (24-hour format with seconds)

local lvgl = require("lvgl")
local colors = require("ts_ui.colors")

local M = {}

-- Active picker instance
local active_picker = nil

--- Show the time picker modal
-- @param options table:
--   - title: string - Title to display (default "Time")
--   - hour: number - Initial hour (0-23, default 12)
--   - minute: number - Initial minute (0-59, default 0)
--   - second: number - Initial second (0-59, default 0)
--   - on_save: function(key, type, value) - Called with {hour, minute, second}
--   - key: string - Setting key to pass to on_save
function M.show(options)
    options = options or {}
    local title = options.title or "Time"
    local init_hour = options.hour or 12
    local init_minute = options.minute or 0
    local init_second = options.second or 0
    local on_save = options.on_save
    local key = options.key or "time"
    
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
    
    overlay:Label {
        text = title,
        text_color = colors.text,
        text_font = lvgl.BUILTIN_FONT.MONTSERRAT_16,
        align = {
            type = lvgl.ALIGN.TOP_MID,
            x_ofs = 0,
            y_ofs = 0,
        },
    }
    
    -- Container for time inputs (centered)
    local input_container = overlay:Object {
        w = lvgl.SIZE_CONTENT,
        h = lvgl.SIZE_CONTENT,
        bg_opa = lvgl.OPA(0),
        outline_width = 0,
        border_width = 0,
        align = lvgl.ALIGN.CENTER,
        flex = {
            flex_direction = "row",
            flex_wrap = "nowrap",
            justify_content = "center",
            align_items = "center",
            align_content = "center",
        },
    }
    
    input_container:clear_flag(lvgl.FLAG.SCROLLABLE)
    
    -- Hour options (00-23 for 24-hour format)
    local hour_options = "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23"
    
    -- Minute/Second options (00-59)
    local minute_options = "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n31\n32\n33\n34\n35\n36\n37\n38\n39\n40\n41\n42\n43\n44\n45\n46\n47\n48\n49\n50\n51\n52\n53\n54\n55\n56\n57\n58\n59"
    
    -- Hour roller
    local hour_roller = input_container:Roller {
        options = { options = hour_options, mode = 0 },
        w = 42,
        h = 40,
        bg_color = colors.card,
        border_width = 0,
        outline_width = 0,
        radius = 8,
    }
    hour_roller:set { selected = { selected = init_hour, anim = 0 } }
    hour_roller:set_style({ bg_opa = lvgl.OPA(0) }, lvgl.PART.SELECTED)
    
    -- Colon separator
    input_container:Label {
        text = ":",
        text_color = colors.text,
        text_font = lvgl.BUILTIN_FONT.MONTSERRAT_24,
        pad_left = 2,
        pad_right = 2,
    }
    
    -- Minute roller
    local minute_roller = input_container:Roller {
        options = { options = minute_options, mode = 0 },
        w = 42,
        h = 40,
        bg_color = colors.card,
        border_width = 0,
        outline_width = 0,
        radius = 8,
    }
    minute_roller:set { selected = { selected = init_minute, anim = 0 } }
    minute_roller:set_style({ bg_opa = lvgl.OPA(0) }, lvgl.PART.SELECTED)
    
    -- Colon separator
    input_container:Label {
        text = ":",
        text_color = colors.text,
        text_font = lvgl.BUILTIN_FONT.MONTSERRAT_24,
        pad_left = 2,
        pad_right = 2,
    }
    
    -- Second roller
    local second_roller = input_container:Roller {
        options = { options = minute_options, mode = 0 },  -- Same 00-59 options
        w = 42,
        h = 40,
        bg_color = colors.card,
        border_width = 0,
        outline_width = 0,
        radius = 8,
    }
    second_roller:set { selected = { selected = init_second, anim = 0 } }
    second_roller:set_style({ bg_opa = lvgl.OPA(0) }, lvgl.PART.SELECTED)
    
    -- Save button
    local save_btn = overlay:Object {
        w = 120,
        h = 40,
        bg_color = colors.primary,
        radius = 10,
        outline_width = 0,
        border_width = 0,
        align = {
            type = lvgl.ALIGN.BOTTOM_MID,
            x_ofs = 0,
            y_ofs = -10,
        },
    }
    save_btn:clear_flag(lvgl.FLAG.SCROLLABLE)
    
    save_btn:Label {
        text = "Set",
        text_color = colors.text,
        align = lvgl.ALIGN.CENTER,
    }
    
    save_btn:onClicked(function()
        -- Get selected values from roller strings
        local hour_str = hour_roller:get_selected_str()
        local minute_str = minute_roller:get_selected_str()
        local second_str = second_roller:get_selected_str()
        local hour = tonumber(hour_str) or 0
        local minute = tonumber(minute_str) or 0
        local second = tonumber(second_str) or 0
        
        local value = {
            hour = hour,
            minute = minute,
            second = second,
        }
        
        M.hide()
        
        if on_save then
            on_save(key, "time", value)
        end
    end)
    
    return overlay
end

--- Hide the time picker
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
