local lvgl = require("lvgl")
local ts_ui = require("ts_ui")
local colors = ts_ui.colors

-- Stopwatch app primary color
local primary_color = colors.primary

-- Timer update interval in milliseconds
local UPDATE_INTERVAL = 100

-- Stopwatch state (loaded from storage)
local start_time = kv.getNumber("sw_strt", 0) or 0   -- Unix timestamp when started (0 = not running)
local accumulated = kv.getNumber("sw_acc", 0) or 0   -- Previously accumulated milliseconds

-- Determine if running based on stored start_time
local is_running = start_time > 0

-- UI elements
local time_label = nil
local start_btn_label = nil
local update_timer = nil

-- Format time as H:MM:SS
local function format_time(ms)
    if ms < 0 then ms = 0 end
    local total_sec = math.floor(ms / 1000)
    local h = math.floor(total_sec / 3600)
    local m = math.floor((total_sec % 3600) / 60)
    local s = total_sec % 60
    return string.format("%d:%02d:%02d", h, m, s)
end

-- Get current elapsed time in milliseconds
local function get_elapsed()
    if is_running and start_time > 0 then
        local now = timeEvents.now()
        local running_seconds = now - start_time
        return accumulated + (running_seconds * 1000)
    else
        return accumulated
    end
end

-- Build UI
LVGL_lock()

local root = lvgl.Object {
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
}

-- Time display
time_label = root:Label {
    text = format_time(get_elapsed()),
    text_color = colors.text,
    text_font = lvgl.BUILTIN_FONT.MONTSERRAT_40,
    align = {
        type = lvgl.ALIGN.CENTER,
        x_ofs = 0,
        y_ofs = -50,
    },
}

-- Start/Stop button
local start_btn = root:Object {
    w = 140,
    h = 50,
    bg_color = primary_color,
    radius = 12,
    outline_width = 0,
    border_width = 0,
    align = {
        type = lvgl.ALIGN.CENTER,
        x_ofs = 0,
        y_ofs = 25,
    },
}
start_btn:clear_flag(lvgl.FLAG.SCROLLABLE)

start_btn_label = start_btn:Label {
    text = is_running and "STOP" or "START",
    text_color = colors.text,
    text_font = lvgl.BUILTIN_FONT.MONTSERRAT_16,
    align = lvgl.ALIGN.CENTER,
}

start_btn:onClicked(function()
    if is_running then
        -- Stop: save accumulated time, clear start_time
        accumulated = get_elapsed()
        start_time = 0
        is_running = false
        kv.setNumber("sw_strt", 0)
        kv.setNumber("sw_acc", accumulated)
        start_btn_label.text = "START"
    else
        -- Start: record current timestamp
        start_time = timeEvents.now()
        is_running = true
        kv.setNumber("sw_strt", start_time)
        start_btn_label.text = "STOP"
    end
end)

-- Reset button
local reset_btn = root:Object {
    w = 100,
    h = 40,
    bg_color = colors.card,
    radius = 10,
    outline_width = 0,
    border_width = 0,
    align = {
        type = lvgl.ALIGN.CENTER,
        x_ofs = 0,
        y_ofs = 85,
    },
}
reset_btn:clear_flag(lvgl.FLAG.SCROLLABLE)

local reset_btn_label = reset_btn:Label {
    text = "Reset",
    text_color = colors.text,
    text_font = lvgl.BUILTIN_FONT.MONTSERRAT_14,
    align = lvgl.ALIGN.CENTER,
}

reset_btn:onClicked(function()
    is_running = false
    start_time = 0
    accumulated = 0
    kv.setNumber("sw_strt", 0)
    kv.setNumber("sw_acc", 0)
    start_btn_label.text = "START"
    time_label.text = format_time(0)
end)

-- Update timer for display
update_timer = lvgl.Timer({
    period = UPDATE_INTERVAL,
    repeat_count = -1,
    cb = function()
        time_label.text = format_time(get_elapsed())
    end,
    paused = false,
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
    if update_timer then
        update_timer:delete()
    end
    root:delete()
end
