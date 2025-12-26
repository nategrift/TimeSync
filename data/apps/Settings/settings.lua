local lvgl = require("lvgl")
local ts_ui = require("ts_ui")
local colors = ts_ui.colors
local icons = ts_ui.icons

-- Initialize settings from system config
local function get_initial_time()
    if config and config.getTime then
        return config.getTime()
    end
    return { hour = 12, minute = 0, second = 0 }
end

local function get_initial_date()
    if config and config.getDate then
        return config.getDate()
    end
    return { day = 1, month = 1, year = 2025 }
end

local function get_initial_string(key, default)
    if watch and watch.getString then
        return watch.getString(key, default)
    end
    return default
end

local function get_initial_int(key, default)
    if watch and watch["get" .. key] then
        return watch["get" .. key]()
    end
    return default
end

local function get_initial_bool(key, default)
    if watch and watch["get" .. key] then
        return watch["get" .. key]()
    end
    return default
end

-- Settings data store
local settings_values = {
    name = get_initial_string("Name", "TimeSync"),
    time = get_initial_time(),
    date = get_initial_date(),
    brightness = get_initial_int("Brightness", 5),
    volume = get_initial_int("Volume", 5),
    mute = get_initial_bool("Mute", false),
}

-- Update a setting value
local function update_setting(key, value)
    settings_values[key] = value
    print("[Settings] Updated " .. key .. " = " .. tostring(value))
end

-- Get a setting value
local function get_setting(key)
    return settings_values[key]
end

-- Format value for display based on type
local function format_value(item)
    local value = get_setting(item.key)
    if item.type == "boolean" then
        return value and "On" or "Off"
    elseif item.type == "time" then
        if type(value) == "table" then
            local h = string.format("%02d", value.hour or 0)
            local m = string.format("%02d", value.minute or 0)
            local s = string.format("%02d", value.second or 0)
            return h .. ":" .. m .. ":" .. s
        end
        return tostring(value or "")
    elseif item.type == "date" then
        if type(value) == "table" then
            local d = string.format("%02d", value.day or 1)
            local m = string.format("%02d", value.month or 1)
            local y = tostring(value.year or 2025)
            return d .. "-" .. m .. "-" .. y
        end
        return tostring(value or "")
    elseif item.type == "action" then
        return ""
    else
        return tostring(value or "")
    end
end

-- Handle save from input modals
local function handle_save(key, input_type, value, value_label, item)
    update_setting(key, value)
    value_label.text = format_value(item)
    
    -- Call system bindings (if available)
    if input_type == "time" and value and config then
        config.setTime(value.hour or 0, value.minute or 0, value.second or 0)
    elseif input_type == "date" and value and config then
        config.setDate(value.year or 2025, value.month or 1, value.day or 1)
    elseif input_type == "string" and watch and item.config_key then
        watch.setString(item.config_key, value)
    elseif input_type == "roller" then
        if key == "brightness" and watch then
            watch.setBrightness(tonumber(value) or 5)
        elseif key == "volume" and watch then
            watch.setVolume(tonumber(value) or 5)
        end
    elseif input_type == "boolean" then
        if key == "mute" and watch then
            watch.setMute(value)
        end
    end
end

-- Settings configuration
local settings_items = {
    {
        key = "name",
        type = "string",
        display_name = "Device Name",
        config_key = "Name",  -- Key used in ConfigManager
    },
    {
        key = "time",
        type = "time",
        display_name = "Time",
    },
    {
        key = "date",
        type = "date",
        display_name = "Date",
    },
    {
        key = "brightness",
        type = "roller",
        display_name = "Brightness",
        values = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10" },
        size = 1,
    },
    {
        key = "volume",
        type = "roller",
        display_name = "Volume",
        values = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10" },
        size = 1,
    },
    {
        key = "mute",
        type = "boolean",
        display_name = "Mute",
    },
    {
        key = "restart",
        type = "action",
        display_name = "Restart Device",
        action = function()
            if watch then
                watch.restart()
            end
        end,
        confirm_text = "Restart",
    },
}

LVGL_lock()

local root = lvgl.Object({
    w = lvgl.HOR_RES(),
    h = lvgl.VER_RES(),
    x = 0,
    y = 0,
    bg_color = colors.background,
    bg_opa = lvgl.OPA(100),
    pad_all = 0,
    outline_width = 0,
    border_width = 0,
    scrollbar_mode = lvgl.SCROLLBAR_MODE.OFF,
})

-- Scrollable container for settings cards
local container = root:Object {
    w = lvgl.HOR_RES(),
    h = lvgl.VER_RES(),
    bg_color = colors.background,
    bg_opa = lvgl.OPA(0),
    outline_width = 0,
    border_width = 0,
    pad_top = 60,
    pad_bottom = 60,
    pad_left = 30,
    pad_right = 30,
    flex = {
        flex_direction = "column",
        flex_wrap = "nowrap",
        justify_content = "flex_start",
        align_items = "center",
        align_content = "center",
    },
    scrollbar_mode = lvgl.SCROLLBAR_MODE.OFF,
}

-- Card dimensions
local card_width = lvgl.HOR_RES() - 80
local card_height = 60

-- Create a setting card
local function create_setting_card(parent, item, index)
    local card = parent:Object {
        w = card_width,
        h = card_height,
        bg_color = colors.card,
        radius = 12,
        outline_width = 0,
        border_width = 0,
        pad_left = 15,
        pad_right = 15,
        pad_top = 0,
        pad_bottom = 0,
    }
    
    card:clear_flag(lvgl.FLAG.SCROLLABLE)
    
    -- Display name (top-left)
    local label = card:Label {
        text = item.display_name,
        text_color = colors.text,
        align = {
            type = lvgl.ALIGN.LEFT_MID,
            x_ofs = 0,
            y_ofs = -10,
        },
    }
    
    -- Value (below label, secondary color)
    local value_label = card:Label {
        text = format_value(item),
        text_color = colors.secondary_text,
        align = {
            type = lvgl.ALIGN.LEFT_MID,
            x_ofs = 0,
            y_ofs = 10,
        },
    }
    
    -- Right arrow indicator (middle-right)
    local arrow = card:Label {
        text = icons.right,
        text_color = colors.secondary_text,
        align = {
            type = lvgl.ALIGN.RIGHT_MID,
            x_ofs = 0,
            y_ofs = 0,
        },
    }
    
    -- Click handler - open appropriate modal based on type
    card:onClicked(function()
        print("[Settings] Clicked: " .. item.key)
        
        if item.type == "time" then
            local current = get_setting(item.key) or {}
            ts_ui.time_picker.show({
                title = item.display_name,
                hour = current.hour or 0,
                minute = current.minute or 0,
                second = current.second or 0,
                key = item.key,
                on_save = function(k, t, v)
                    handle_save(k, t, v, value_label, item)
                end,
            })
        elseif item.type == "date" then
            local current = get_setting(item.key) or {}
            ts_ui.date_picker.show({
                title = item.display_name,
                day = current.day or 1,
                month = current.month or 1,
                year = current.year or 2025,
                key = item.key,
                on_save = function(k, t, v)
                    handle_save(k, t, v, value_label, item)
                end,
            })
        elseif item.type == "string" then
            local current = get_setting(item.key) or ""
            ts_ui.text_input.show({
                title = item.display_name,
                value = current,
                key = item.key,
                on_save = function(k, t, v)
                    handle_save(k, t, v, value_label, item)
                end,
            })
        elseif item.type == "roller" then
            local current = get_setting(item.key)
            ts_ui.roller_picker.show({
                title = item.display_name,
                values = item.values or {},
                selected = tostring(current),
                size = item.size or 2,
                key = item.key,
                on_save = function(k, t, v)
                    handle_save(k, t, v, value_label, item)
                end,
            })
        elseif item.type == "boolean" then
            local current = get_setting(item.key) or false
            ts_ui.bool_picker.show({
                title = item.display_name,
                value = current,
                key = item.key,
                on_save = function(k, t, v)
                    handle_save(k, t, v, value_label, item)
                end,
            })
        elseif item.type == "action" then
            ts_ui.action_picker.show({
                title = item.display_name,
                message = item.message or "",
                confirm_text = item.confirm_text or "Confirm",
                on_confirm = item.action,
            })
        end
    end)
    
    return card, value_label
end

-- Build all setting cards
local card_refs = {}
for i, item in ipairs(settings_items) do
    local card, value_label = create_setting_card(container, item, i)
    card_refs[item.key] = {
        card = card,
        value_label = value_label,
        item = item,
    }
end

-- Swipe to show app selector
local screen = lvgl.disp.get_scr_act()
screen:onevent(lvgl.EVENT.GESTURE, function(obj, code)
    local indev = lvgl.indev.get_act()
    local dir = indev:get_gesture_dir()
    if dir == lvgl.DIR.LEFT or dir == lvgl.DIR.RIGHT then
        ts_ui.app_selector.show()
    end
end)

LVGL_unlock()

-- Cleanup on close
OnClose = function()
    root:delete()
end
