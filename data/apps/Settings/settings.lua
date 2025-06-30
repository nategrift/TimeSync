local lvgl = require("lvgl")

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
})

local button_w = 100
local button_h = 60

local button = root:Object {
    w = button_w,
    h = button_h,
    x = lvgl.HOR_RES() / 2 - button_w / 2,
    y = 10 + button_h / 2,
    bg_color = "#5e82d6",
    border_width = 0,
    radius = 5,
    pad_all = 0,
    outline_width = 0,
}


local label = button:Label {
    text = "Settings",
    text_color = "#1f1f1f",
    align = lvgl.ALIGN.CENTER,
    font = lvgl.BUILTIN_FONT.MONTSERRAT_42,
}

button:onClicked(function()
    print("clicked")
    openApp("Clock")
end)

local timer = lvgl.Timer({
    period = 1000,
    repeat_count = -1,
    cb = function()
        print("timer")
        local date = os.date("%Y-%m-%d %H:%M:%S")
        print("current Date: " .. date)

    end,
    paused = false
})

local container = root:Object {
    w = lvgl.HOR_RES(),
    h = lvgl.SIZE_CONTENT,
    bg_color = "#000",
    outline_width = 0,
    border_width = 0,
    pad_ver = lvgl.VER_RES() / 2,
    flex = {
        flex_direction = "column",
        flex_wrap = "nowrap",
        justify_content = "center",
        align_items = "center",
        align_content = "center",
    },
}

local home = container:Object {
    bg_color = lvgl.palette.darken(lvgl.palette.RED, 3),
    radius = 5,
    outline_width = 0,
    border_width = 0,
    w = lvgl.HOR_RES() - 60,
    h = 32,
    pad_bottom = 10,
}    

home:clear_flag(lvgl.FLAG.SCROLLABLE)

home:Label {
    text_color = "#fff",
    text = "Home",
    align = lvgl.ALIGN.CENTER,
    pad_all = 0,
}

home:onClicked(function()
    openApp("Clock")
end)

for i = 1, 10 do
    local item = container:Object {
        bg_color = "#444",
        radius = 5,
        outline_width = 0,
        border_width = 0,
        w = lvgl.HOR_RES() - 60,
        h = 32,
        pad_bottom = 10,
    }    
    
    item:clear_flag(lvgl.FLAG.SCROLLABLE)

    item:Label {
        text_color = "#fff",
        text = "Option " .. i,
        align = lvgl.ALIGN.CENTER,
        pad_all = 0,
    }
end

-- button:onClicked(function()
--     root:delete()
--     close()
-- end)

local screen = lvgl.disp.get_scr_act()
local press_start_time = nil
screen:onevent(lvgl.EVENT.GESTURE, function (obj, code)
    local indev = lvgl.indev.get_act()
    if press_start_time then
        local press_duration = os.clock() - press_start_time
        if press_duration > 0.8 then
            if indev:get_gesture_dir() == lvgl.DIR.LEFT then
                openApp("Clock")
            end
        end
    end
end)

root:onevent(lvgl.EVENT.PRESSED, function (obj, code)
    press_start_time = os.clock()
end)

