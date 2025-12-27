-- ts_ui.app_selector
-- App selector overlay for TimeSync apps

local lvgl = require("lvgl")

local M = {}

--- Launch the circular app selector overlay
-- Shows all installed apps in a ring around a central clock display
-- NOTE: This is called from event callbacks (gestures, presses) which run inside
-- lv_timer_handler(). The mutex is already held, so we do NOT lock here.
function M.show()
    local root = lvgl.Object({
        w = lvgl.HOR_RES(),
        h = lvgl.VER_RES(),
        x = 0,
        y = 0,
        bg_color = "#000",
        bg_opa = lvgl.OPA(100),
        pad_all = 0,
        outline_width = 0,
        border_width = 0,
        scrollbar_mode = lvgl.SCROLLBAR_MODE.OFF,
    })

    local apps = { "Clock", "Settings", "Fitness", "Games", "Calculator", "Timer", "StopWatch", "Alarm" }

    local center_x = lvgl.HOR_RES() // 2
    local center_y = lvgl.VER_RES() // 2
    local radius = 180 / 2

    local indicator_radius = 60
    local indicator_size_radius = 2

    local icon_size = 50
    local icon_offset = icon_size / 2

    local current_app = (currentApp and currentApp.name and currentApp.name()) or "Clock"

    for i, app in ipairs(apps) do
        local angle = (i - 1) * (2 * math.pi / #apps) - math.pi / 2
        local icon_x = center_x + (math.cos(angle) * radius) - icon_offset
        local icon_y = center_y + (math.sin(angle) * radius) - icon_offset

        local btn = root:Object {
            x = icon_x,
            y = icon_y,
            w = icon_size,
            h = icon_size,
            outline_width = 0,
            border_width = 0,
            pad_all = 0,
            bg_opa = lvgl.OPA(0),
            scale = 0,
        }

        local img = btn:Image {
            src = "A:/spiffs/apps/" .. app .. "/icon.png",
            x = 0,
            y = 0,
            opa = lvgl.OPA(0),
        }

        -- Staggered fade-in animation
        local delay = (i - 1) * (700 / #apps) + 100
        img:Anim {
            run = true,
            start_value = 0,
            end_value = 100,
            duration = 200,
            delay = delay,
            path = "ease_out",
            exec_cb = function(obj, value)
                obj.opa = lvgl.OPA(value)
            end,
        }

        btn:onevent(lvgl.EVENT.PRESSED, function(obj, code)
            openApp(app)
        end)

        -- Show indicator for current app
        if current_app == app then
            local indicator_x = center_x + (math.cos(angle) * indicator_radius) - indicator_size_radius
            local indicator_y = center_y + (math.sin(angle) * indicator_radius) - indicator_size_radius
            local indicator = root:Object {
                x = indicator_x,
                y = indicator_y,
                w = indicator_size_radius + indicator_size_radius,
                h = indicator_size_radius + indicator_size_radius,
                bg_color = lvgl.palette.white(),
                border_width = 0,
                radius = indicator_radius,
                pad_all = 0,
                outline_width = 0,
                opa = lvgl.OPA(0),
            }

            indicator:Anim {
                run = true,
                start_value = 0,
                end_value = 100,
                duration = 200,
                delay = delay + 100,
                path = "ease_out",
                exec_cb = function(obj, value)
                    obj.opa = lvgl.OPA(value)
                end
            }
        end
    end

    -- Central time display (no timer needed - overlay is momentary)
    local time_label = root:Label {
        text = tostring(os.date("%I:%M")),
        text_color = "#fff",
        text_font = lvgl.BUILTIN_FONT.MONTSERRAT_24,
        text_align = lvgl.ALIGN.CENTER,
        align = lvgl.ALIGN.CENTER
    }
    
    return root
end

return M

