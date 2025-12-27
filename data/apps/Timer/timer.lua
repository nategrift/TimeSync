local lvgl = require("lvgl")
local ts_ui = require("ts_ui")
local colors = ts_ui.colors

-- Timer app primary color
local primary_color = colors.primary

-- Event type constants
local EVENT_TYPE_TIMER = 1

-- Timer state
local timer_duration = { hour = 0, minute = 5, second = 0 }  -- Default 5 minutes
local active_event_id = nil
local is_running = false

-- Load saved duration from persistent storage
local function load_saved_duration()
    local saved_hour = kv.getNumber("timer_hour", 0)
    local saved_minute = kv.getNumber("timer_min", 5)
    local saved_second = kv.getNumber("timer_sec", 0)
    timer_duration.hour = saved_hour
    timer_duration.minute = saved_minute
    timer_duration.second = saved_second
end

-- Save duration to persistent storage
local function save_duration()
    kv.setNumber("timer_hour", timer_duration.hour)
    kv.setNumber("timer_min", timer_duration.minute)
    kv.setNumber("timer_sec", timer_duration.second)
end

-- UI elements
local time_label = nil
local start_btn_label = nil
local edit_btn = nil
local edit_btn_label = nil
local update_timer = nil

-- Get active timer event
local function get_active_timer()
    local events = timeEvents.getAllByType(EVENT_TYPE_TIMER)
    if #events > 0 then
        return events[1]
    end
    return nil
end

-- Format time string (only show hours if > 0)
local function format_time(total_seconds)
    if total_seconds < 0 then total_seconds = 0 end
    
    local hours = math.floor(total_seconds / 3600)
    local minutes = math.floor((total_seconds % 3600) / 60)
    local seconds = total_seconds % 60
    
    if hours > 0 then
        return string.format("%d:%02d:%02d", hours, minutes, seconds)
    else
        return string.format("%d:%02d", minutes, seconds)
    end
end

-- Calculate remaining time from active event
local function get_remaining_time()
    local event = get_active_timer()
    if event then
        local now = timeEvents.now()
        local remaining = event.expireTime - now
        return math.max(0, remaining)
    end
    return 0
end

-- Update the timer display
local function update_display()
    if is_running then
        local remaining = get_remaining_time()
        time_label.text = format_time(remaining)
        
        -- Check if timer has expired
        if remaining <= 0 then
            is_running = false
            active_event_id = nil
            start_btn_label.text = "Start"
            edit_btn_label.text = "Edit"
            edit_btn:clear_flag(lvgl.FLAG.HIDDEN)
        end
    else
        -- Show the set duration
        local total = timer_duration.hour * 3600 + timer_duration.minute * 60 + timer_duration.second
        time_label.text = format_time(total)
    end
end

-- Start the timer
local function start_timer()
    local total_seconds = timer_duration.hour * 3600 + timer_duration.minute * 60 + timer_duration.second
    if total_seconds <= 0 then
        return
    end
    
    local expire_time = timeEvents.now() + total_seconds
    active_event_id = timeEvents.add(EVENT_TYPE_TIMER, expire_time, "Timer", "Timer")
    is_running = true
    
    start_btn_label.text = "Pause"
    edit_btn_label.text = "Stop"
    edit_btn:clear_flag(lvgl.FLAG.HIDDEN)
    
    update_display()
end

-- Stop the timer
local function stop_timer()
    if active_event_id then
        timeEvents.delete(active_event_id)
        active_event_id = nil
    end
    is_running = false
    
    start_btn_label.text = "Start"
    edit_btn_label.text = "Edit"
    
    update_display()
end

-- Pause the timer (save remaining time as new duration)
local function pause_timer()
    local remaining = get_remaining_time()
    
    -- Save remaining time as new duration
    timer_duration.hour = math.floor(remaining / 3600)
    timer_duration.minute = math.floor((remaining % 3600) / 60)
    timer_duration.second = remaining % 60
    
    -- Delete the active event
    if active_event_id then
        timeEvents.delete(active_event_id)
        active_event_id = nil
    end
    is_running = false
    
    start_btn_label.text = "Start"
    edit_btn_label.text = "Edit"
    
    update_display()
end

-- Open time picker to edit duration
local function edit_timer()
    ts_ui.time_picker.show({
        title = "Timer",
        hour = timer_duration.hour,
        minute = timer_duration.minute,
        second = timer_duration.second,
        key = "timer_duration",
        on_save = function(key, type, value)
            timer_duration.hour = value.hour or 0
            timer_duration.minute = value.minute or 0
            timer_duration.second = value.second or 0
            save_duration()  -- Persist the new duration
            update_display()
        end,
    })
end

-- Initialize state from existing events
local function init_state()
    local event = get_active_timer()
    if event then
        active_event_id = event.id
        local remaining = event.expireTime - timeEvents.now()
        if remaining > 0 then
            is_running = true
        else
            -- Event has expired, clean it up
            timeEvents.delete(event.id)
            active_event_id = nil
            is_running = false
            load_saved_duration()  -- Load last set time when no timer running
        end
    else
        -- No active timer, load the saved duration
        load_saved_duration()
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

-- Timer display
time_label = root:Label {
    text = "0:00",
    text_color = colors.text,
    text_font = lvgl.BUILTIN_FONT.MONTSERRAT_48,
    align = {
        type = lvgl.ALIGN.CENTER,
        x_ofs = 0,
        y_ofs = -50,
    },
}

-- Start/Pause button
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
    text = "Start",
    text_color = colors.text,
    text_font = lvgl.BUILTIN_FONT.MONTSERRAT_16,
    align = lvgl.ALIGN.CENTER,
}

start_btn:onClicked(function()
    if is_running then
        pause_timer()
    else
        start_timer()
    end
end)

-- Edit/Stop button
edit_btn = root:Object {
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
edit_btn:clear_flag(lvgl.FLAG.SCROLLABLE)

edit_btn_label = edit_btn:Label {
    text = "Edit",
    text_color = colors.text,
    text_font = lvgl.BUILTIN_FONT.MONTSERRAT_14,
    align = lvgl.ALIGN.CENTER,
}

edit_btn:onClicked(function()
    if is_running then
        stop_timer()
    else
        edit_timer()
    end
end)

-- Initialize state
init_state()

-- Update button labels based on state
if is_running then
    start_btn_label.text = "Pause"
    edit_btn_label.text = "Stop"
else
    start_btn_label.text = "Start"
    edit_btn_label.text = "Edit"
end

-- Initial display update
update_display()

-- Update timer every second
update_timer = lvgl.Timer({
    period = 1000,
    repeat_count = -1,
    cb = function()
        update_display()
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
