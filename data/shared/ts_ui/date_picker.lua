local lvgl = require("lvgl")
local colors = require("ts_ui.colors")

local M = {}

local active_picker = nil

function M.show(options)
    options = options or {}
    local title = options.title or "Date"
    local init_day = options.day or 1
    local init_month = options.month or 1
    local init_year = options.year or 2025
    local on_save = options.on_save
    local key = options.key or "date"
    
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
    
    -- Container for date inputs (centered)
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
    
    -- Day options (01-31)
    local day_options = "01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n31"
    
    -- Month options (01-12)
    local month_options = "01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12"
    
    -- Year options (2020-2035)
    local year_options = "2020\n2021\n2022\n2023\n2024\n2025\n2026\n2027\n2028\n2029\n2030\n2031\n2032\n2033\n2034\n2035"
    
    -- Calculate year index (offset from 2020)
    local year_index = init_year - 2020
    if year_index < 0 then year_index = 5 end  -- Default to 2025
    if year_index > 15 then year_index = 15 end
    
    -- Day roller
    local day_roller = input_container:Roller {
        options = { options = day_options, mode = 0 },
        w = 45,
        h = 40,
        bg_color = colors.card,
        border_width = 0,
        outline_width = 0,
        radius = 8,
    }
    day_roller:set { selected = { selected = init_day - 1, anim = 0 } }
    day_roller:set_style({ bg_opa = lvgl.OPA(0) }, lvgl.PART.SELECTED)
    
    -- Month roller
    local month_roller = input_container:Roller {
        options = { options = month_options, mode = 0 },
        w = 45,
        h = 40,
        bg_color = colors.card,
        border_width = 0,
        outline_width = 0,
        radius = 8,
    }
    month_roller:set { selected = { selected = init_month - 1, anim = 0 } }
    month_roller:set_style({ bg_opa = lvgl.OPA(0) }, lvgl.PART.SELECTED)

    -- Year roller
    local year_roller = input_container:Roller {
        options = { options = year_options, mode = 0 },
        w = 55,
        h = 40,
        bg_color = colors.card,
        border_width = 0,
        outline_width = 0,
        radius = 8,
    }
    year_roller:set { selected = { selected = year_index, anim = 0 } }
    year_roller:set_style({ bg_opa = lvgl.OPA(0) }, lvgl.PART.SELECTED)
    
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
        local day_str = day_roller:get_selected_str()
        local month_str = month_roller:get_selected_str()
        local year_str = year_roller:get_selected_str()
        local day = tonumber(day_str) or 1
        local month = tonumber(month_str) or 1
        local year = tonumber(year_str) or 2025
        
        local value = {
            day = day,
            month = month,
            year = year,
        }
        
        M.hide()
        
        if on_save then
            on_save(key, "date", value)
        end
    end)
    
    return overlay
end

--- Hide the date picker
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


