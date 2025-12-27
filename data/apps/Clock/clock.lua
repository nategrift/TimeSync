local lvgl = require("lvgl")
local ts_ui = require("ts_ui")
local colors = ts_ui.colors
local icons = ts_ui.icons

LVGL_lock()
local root = lvgl.Object {
    w = lvgl.HOR_RES(),
    h = lvgl.VER_RES(),
    x = 0,
    y = 0,
    bg_color = "#000",
    bg_opa = lvgl.OPA(100),
    pad_all = 0,
    outline_width = 0,
    border_width = 0,
}

-- local img = root:Image {
--     src = "A:/spiffs/apps/Clock/portraitTiny.png",
--     x = 0,
--     y = 0,
-- }

local time = root:Label {
    text = "--/--/--",
    text_color = "#fff",
    text_font = lvgl.BUILTIN_FONT.NERD_FONT,
    text_align = lvgl.ALIGN.CENTER,
    align = lvgl.ALIGN.CENTER
}


local date = root:Label {
    text = "00:00:00",
    text_color = lvgl.palette.lighten(lvgl.palette.GREY, 2),
    text_font = lvgl.BUILTIN_FONT.MONTSERRAT_16,
    align = {
        type = lvgl.ALIGN.CENTER,
        x_ofs = 0,
        y_ofs = 30,
    }
}

local seconds_dot = root:Object {
    w = 4,
    h = 4,
    radius = 2,
    bg_color = colors.primary,
    bg_opa = lvgl.OPA(100),
    x = 120,
    y = 5,
}

-- Smooth position counter (increments every 100ms, wraps at 600 = 60 seconds)
local position_counter = 0
local synced = false

-- Update seconds indicator position with smooth interpolation
local function update_seconds_indicator()
    local sec = tonumber(os.date("%S")) or 0
    
    -- Sync once at start, then just increment smoothly
    if not synced then
        position_counter = sec * 10
        synced = true
    else
        -- Always increment by 1 (100ms = 1/10 second)
        position_counter = position_counter + 1
        -- Wrap at 600 (60 seconds * 10 steps)
        if position_counter >= 600 then
            position_counter = 0
        end
    end
    
    -- Calculate smooth second value (0-60)
    local smooth_sec = position_counter / 10
    
    -- Calculate angle (start from top, go clockwise)
    local angle = (smooth_sec / 60) * 2 * math.pi - (math.pi / 2)
    local radius = 115  -- Distance from center
    local center_x = 120
    local center_y = 120
    
    -- Calculate position (offset by half dot size for centering)
    local x = center_x + radius * math.cos(angle) - 2
    local y = center_y + radius * math.sin(angle) - 2
    
    seconds_dot.x = math.floor(x)
    seconds_dot.y = math.floor(y)
end

-- WiFi status icon (left of center when shown, hidden by default)
local wifi_icon = root:Label {
    text = "",  -- Start hidden
    text_color = colors.status_off,
    align = {
        type = lvgl.ALIGN.BOTTOM_MID,
        x_ofs = -15,
        y_ofs = -15,
    },
}

-- Battery status icon (centered by default, moves right when wifi shown)
local battery_icon = root:Label {
    text = icons.battery_full,
    text_color = colors.status_off,
    align = {
        type = lvgl.ALIGN.BOTTOM_MID,
        x_ofs = 0,  -- Centered by default
        y_ofs = -15,
    },
}

-- Track if WiFi icon is currently visible
local wifi_icon_visible = false

-- Update WiFi icon based on connection status
local function update_wifi_icon()
    -- Hide WiFi icon if WiFi is not enabled
    if not wifi or not wifi.isOn or not wifi.isOn() then
        if wifi_icon_visible then
            wifi_icon.text = ""  -- Clear text to hide
            wifi_icon_visible = false
            -- Center battery icon when WiFi is hidden
            battery_icon:set { align = { type = lvgl.ALIGN.BOTTOM_MID, x_ofs = 0, y_ofs = -15 } }
        end
        return
    end
    
    -- WiFi is enabled - show icon
    if not wifi_icon_visible then
        wifi_icon.text = icons.wifi
        wifi_icon_visible = true
        -- Move battery icon to the right when WiFi is shown
        battery_icon:set { align = { type = lvgl.ALIGN.BOTTOM_MID, x_ofs = 15, y_ofs = -15 } }
    end
    
    if not wifi.isConnected or not wifi.isConnected() then
        wifi_icon.text_color = colors.status_off
        return
    end
    
    -- Connected - color based on signal strength
    local rssi = wifi.getSignalStrength and wifi.getSignalStrength() or -100
    
    if rssi >= -60 then
        wifi_icon.text_color = colors.status_good
    elseif rssi >= -70 then
        wifi_icon.text_color = colors.status_warn
    else
        wifi_icon.text_color = colors.status_bad
    end
end

-- Update Battery icon based on level and charging status
local function update_battery_icon()
    if not battery then
        battery_icon.text_color = colors.status_off
        return
    end
    
    local charging = battery.isCharging and battery.isCharging() or false
    local level = battery.getLevel and battery.getLevel() or 0
    
    -- Set icon based on charging or level
    if charging then
        battery_icon.text = icons.charge
        battery_icon.text_color = colors.status_good
        return
    end
    
    -- Set icon based on level
    if level >= 80 then
        battery_icon.text = icons.battery_full
        battery_icon.text_color = colors.status_good
    elseif level >= 60 then
        battery_icon.text = icons.battery_3
        battery_icon.text_color = colors.status_good
    elseif level >= 40 then
        battery_icon.text = icons.battery_2
        battery_icon.text_color = colors.status_warn
    elseif level >= 20 then
        battery_icon.text = icons.battery_1
        battery_icon.text_color = colors.status_warn
    else
        battery_icon.text = icons.battery_empty
        battery_icon.text_color = colors.status_bad
    end
end

local function update_time()
    local timeString = tostring(os.date("%I:%M"))
    local dateString = tostring(os.date("%a %b %d"))
    time.text = timeString
    date.text = dateString
end
update_time()
update_wifi_icon()
update_battery_icon()
update_seconds_indicator()

-- Main timer for time/date/status updates (every 1 second)
local timer = lvgl.Timer({
    period = 1000,
    repeat_count = -1,
    cb = function()
        update_time()
        update_wifi_icon()
        update_battery_icon()
    end,
    paused = false
})

-- Fast timer for smooth seconds indicator (every 100ms)
local seconds_timer = lvgl.Timer({
    period = 100,
    repeat_count = -1,
    cb = function()
        update_seconds_indicator()
    end,
    paused = false
})

-- Gesture on screen for swipe detection
local screen = lvgl.disp.get_scr_act()
screen:onevent(lvgl.EVENT.GESTURE, function(obj, code)
    local indev = lvgl.indev.get_act()
    local dir = indev:get_gesture_dir()
    if dir == lvgl.DIR.LEFT or dir == lvgl.DIR.RIGHT then
        ts_ui.app_selector.show()
    end
end)

root:onevent(lvgl.EVENT.PRESSED, function(obj, code)
    ts_ui.app_selector.show()
end)

LVGL_unlock()

OnClose = function()
    timer:delete()
    seconds_timer:delete()
    root:delete()
end