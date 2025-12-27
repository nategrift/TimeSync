local lvgl = require("lvgl")
local ts_ui = require("ts_ui")
local colors = ts_ui.colors

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
    wifi_enabled = wifi and wifi.isEnabled() or false,
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
    elseif item.type == "action" or item.type == "submenu" then
        return ""
    else
        return tostring(value or "")
    end
end

-- Save network and connect
local function select_network(ssid, password)
    print("[Settings] Selecting network: " .. ssid)
    if wifi and wifi.selectNetwork then
        wifi.selectNetwork(ssid, password or "")
    end
end

-- Networks submenu state
local networks_refresh_timer = nil
local networks_submenu_open = false

-- Build network items from cached scan results
local function build_network_items()
    local networks = {}
    if wifi and wifi.getScannedNetworks then
        networks = wifi.getScannedNetworks()
    end
    
    local items = {}
    
    -- Check if scan is in progress
    local scanning = wifi and wifi.isScanInProgress and wifi.isScanInProgress()
    
    if scanning and #networks == 0 then
        items[1] = {
            display_name = "Scanning...",
            value = "Looking for networks",
            show_arrow = false,
        }
    elseif #networks == 0 then
        items[1] = {
            display_name = "No networks found",
            value = "Make sure WiFi is enabled",
            show_arrow = false,
        }
    else
        for i, network in ipairs(networks) do
            items[i] = {
                display_name = network.ssid,
                value = network.rssi .. "dBm; " .. network.authMode,
                show_arrow = true,
                on_click = function(item, index)
                    print("[Settings] Selected network: " .. network.ssid)
                    
                    -- Stop refresh timer when selecting a network
                    if networks_refresh_timer then
                        networks_refresh_timer:delete()
                        networks_refresh_timer = nil
                    end
                    networks_submenu_open = false
                    
                    if network.isOpen then
                        -- Open network, connect directly without password
                        ts_ui.sub_pick.hide()
                        select_network(network.ssid, "")
                    else
                        -- Network requires password, show text input
                        ts_ui.text_input.show({
                            title = "Password for " .. network.ssid,
                            value = "",
                            key = "wifi_password",
                            on_save = function(key, type, password)
                                -- Close the networks submenu and connect
                                ts_ui.sub_pick.hide()
                                select_network(network.ssid, password)
                            end,
                        })
                    end
                end,
            }
        end
    end
    
    return items
end

-- Show the Networks list (available WiFi networks)
local function show_networks_submenu()
    networks_submenu_open = true
    
    -- Start a scan when opening the menu
    if wifi and wifi.startScan then
        wifi.startScan()
    end
    
    -- Build initial items
    local items = build_network_items()
    
    ts_ui.sub_pick.show({
        title = "Networks",
        items = items,
        on_back = function()
            -- Stop refresh timer when closing
            if networks_refresh_timer then
                networks_refresh_timer:delete()
                networks_refresh_timer = nil
            end
            networks_submenu_open = false
        end,
    })
    
    -- Set up a timer to refresh the network list every 2 seconds
    networks_refresh_timer = lvgl.Timer({
        period = 2000,
        cb = function()
            if not networks_submenu_open then
                -- Stop timer if submenu was closed
                if networks_refresh_timer then
                    networks_refresh_timer:delete()
                    networks_refresh_timer = nil
                end
                return
            end
            
            -- Rebuild and refresh the submenu with latest networks
            local new_items = build_network_items()
            
            -- Close current submenu and reopen with new items
            ts_ui.sub_pick.hide()
            ts_ui.sub_pick.show({
                title = "Networks",
                items = new_items,
                on_back = function()
                    if networks_refresh_timer then
                        networks_refresh_timer:delete()
                        networks_refresh_timer = nil
                    end
                    networks_submenu_open = false
                end,
            })
        end,
        paused = false,
    })
end

-- Network submenu state
local network_submenu_open = false
local network_status_timer = nil

-- Get network status string with signal strength
local function get_network_status()
    if not wifi then return "WiFi unavailable" end
    
    if not wifi.isOn or not wifi.isOn() then
        return "WiFi is off"
    end
    
    if wifi.isConnected and wifi.isConnected() then
        local ssid = wifi.getConnectedSSID and wifi.getConnectedSSID() or "Unknown"
        local rssi = wifi.getSignalStrength and wifi.getSignalStrength() or 0
        return ssid .. " (" .. rssi .. " dBm)"
    end
    
    return "Disconnected"
end

-- Show the Network settings submenu
local function show_network_submenu()
    network_submenu_open = true
    
    local items = {
        {
            display_name = "WiFi Enabled",
            value = function()
                if wifi and wifi.isOn then
                    return wifi.isOn() and "On" or "Off"
                end
                return "Off"
            end,
            show_arrow = true,
            on_click = function(item, index)
                local current = wifi and wifi.isOn and wifi.isOn() or false
                if wifi and wifi.setEnabled then
                    wifi.setEnabled(not current)
                end
            end,
        },
        {
            display_name = "Status",
            value = get_network_status,
            show_arrow = false,
        },
        {
            display_name = "Networks",
            value = "Scan & connect",
            show_arrow = true,
            on_click = function(item, index)
                if network_status_timer then
                    network_status_timer:delete()
                    network_status_timer = nil
                end
                show_networks_submenu()
            end,
        },
    }
    
    ts_ui.sub_pick.show({
        title = "Network",
        items = items,
        on_back = function()
            network_submenu_open = false
            if network_status_timer then
                network_status_timer:delete()
                network_status_timer = nil
            end
        end,
    })
    
    -- Just refresh values every 1 second, don't recreate menu
    network_status_timer = lvgl.Timer({
        period = 1000,
        cb = function()
            if not network_submenu_open then
                if network_status_timer then
                    network_status_timer:delete()
                    network_status_timer = nil
                end
                return
            end
            ts_ui.sub_pick.refresh_all()
        end,
        paused = false,
    })
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
        key = "network",
        type = "submenu",
        display_name = "Network",
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

-- Handle click on a settings item
local function handle_click(item, index, list_controller)
    print("[Settings] Clicked: " .. item.key)
    
    local function on_save_handler(k, t, v)
        -- Update the setting value
        update_setting(k, v)
        -- Update the list display
        list_controller:update_item(index)
        
        -- Call system bindings (if available)
        if t == "time" and v and config then
            config.setTime(v.hour or 0, v.minute or 0, v.second or 0)
        elseif t == "date" and v and config then
            config.setDate(v.year or 2025, v.month or 1, v.day or 1)
        elseif t == "string" and watch and item.config_key then
            watch.setString(item.config_key, v)
        elseif t == "roller" then
            if k == "brightness" and watch then
                watch.setBrightness(tonumber(v) or 5)
            elseif k == "volume" and watch then
                watch.setVolume(tonumber(v) or 5)
            end
        elseif t == "boolean" then
            if k == "mute" and watch then
                watch.setMute(v)
            end
        end
    end
    
    if item.type == "time" then
        local current = get_setting(item.key) or {}
        ts_ui.time_picker.show({
            title = item.display_name,
            hour = current.hour or 0,
            minute = current.minute or 0,
            second = current.second or 0,
            key = item.key,
            on_save = on_save_handler,
        })
    elseif item.type == "date" then
        local current = get_setting(item.key) or {}
        ts_ui.date_picker.show({
            title = item.display_name,
            day = current.day or 1,
            month = current.month or 1,
            year = current.year or 2025,
            key = item.key,
            on_save = on_save_handler,
        })
    elseif item.type == "string" then
        local current = get_setting(item.key) or ""
        ts_ui.text_input.show({
            title = item.display_name,
            value = current,
            key = item.key,
            on_save = on_save_handler,
        })
    elseif item.type == "roller" then
        local current = get_setting(item.key)
        ts_ui.roller_picker.show({
            title = item.display_name,
            values = item.values or {},
            selected = tostring(current),
            size = item.size or 2,
            key = item.key,
            on_save = on_save_handler,
        })
    elseif item.type == "boolean" then
        local current = get_setting(item.key) or false
        ts_ui.bool_picker.show({
            title = item.display_name,
            value = current,
            key = item.key,
            on_save = on_save_handler,
        })
    elseif item.type == "action" then
        ts_ui.action_picker.show({
            title = item.display_name,
            message = item.message or "",
            confirm_text = item.confirm_text or "Confirm",
            on_confirm = item.action,
        })
    elseif item.type == "submenu" then
        if item.key == "network" then
            show_network_submenu()
        end
    end
end

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

-- Build list items from settings configuration
local list_items = {}
local settings_list  -- Forward declaration for closure

for i, setting in ipairs(settings_items) do
    list_items[i] = {
        key = setting.key,
        type = setting.type,
        display_name = setting.display_name,
        values = setting.values,
        size = setting.size,
        config_key = setting.config_key,
        message = setting.message,
        confirm_text = setting.confirm_text,
        action = setting.action,
        value = function(item)
            return format_value(setting)
        end,
        on_click = function(item, index)
            handle_click(item, index, settings_list)
        end,
    }
end

-- Create the settings list
settings_list = ts_ui.list.create(root, {
    items = list_items,
})

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
    settings_list:delete()
    root:delete()
end
