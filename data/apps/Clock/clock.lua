local lvgl = require("lvgl")

local other = require("other")

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

local screen = lvgl.disp.get_scr_act()
local press_start_time = nil
screen:onevent(lvgl.EVENT.GESTURE, function (obj, code)
    local indev = lvgl.indev.get_act()
    if press_start_time then
        local press_duration = os.clock() - press_start_time
        if press_duration > 0.8 then
            if indev:get_gesture_dir() == lvgl.DIR.LEFT then
                if other.Launch_App_Selector then
                    other.Launch_App_Selector()
                else
                    openApp("Clock")
                end
            end
        end
    end
end)

root:onevent(lvgl.EVENT.PRESSED, function (obj, code)
    -- press_start_time = os.clock()
    if other.Launch_App_Selector then
        other.Launch_App_Selector()
    end
end)


LVGL_unlock()

-- OnClose = function()
--     timer:delete()
--     other.OnClose()
-- end