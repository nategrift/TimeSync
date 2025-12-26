local lvgl = require("lvgl")
local ts_ui = require("ts_ui")

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

local function update_time()
    local timeString = tostring(os.date("%I:%M"))
    local dateString = tostring(os.date("%a %b %d"))
    time.text = timeString
    date.text = dateString
end
update_time()

local timer = lvgl.Timer({
    period = 1000,
    repeat_count = -1,
    cb = function()
        update_time()
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
    root:delete()
end