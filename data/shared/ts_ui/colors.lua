-- ts_ui.colors
-- Color palette for TimeSync apps

local M = {}

-- Primary color - gets from current app config dynamically
-- Access as: colors.primary (uses metatable for dynamic lookup)
local DEFAULT_PRIMARY = "#E9444C"

setmetatable(M, {
    __index = function(t, k)
        if k == "primary" then
            -- Try to get from currentApp, fallback to default
            if currentApp == nil then
                print("[colors] currentApp is nil, using default")
                return DEFAULT_PRIMARY
            end
            if type(currentApp) ~= "table" then
                print("[colors] currentApp is not a table, type=" .. type(currentApp))
                return DEFAULT_PRIMARY
            end
            if currentApp.primaryColor == nil then
                print("[colors] currentApp.primaryColor is nil")
                return DEFAULT_PRIMARY
            end
            local ok, color = pcall(currentApp.primaryColor)
            if ok and color then
                return color
            end
            print("[colors] pcall failed or color nil")
            return DEFAULT_PRIMARY
        end
        return rawget(t, k)
    end
})

-- Card backgrounds
M.card = "#3E3E3E"

-- Text colors
M.text = "#FFFFFF"
M.secondary_text = "#B3B3B3"

-- Background
M.background = "#000000"

-- Button colors (for keyboards, calculators, etc.)
M.key = "#4D5B68"
M.modifier = "#788B9C"

-- Status indicator colors (for WiFi, battery, etc.)
M.status_off = "#555555"      -- Grey - off/inactive
M.status_bad = "#B35555"      -- Muted red - bad/low
M.status_warn = "#B3A355"     -- Muted yellow - warning/medium
M.status_good = "#55B355"     -- Muted green - good/high

return M

