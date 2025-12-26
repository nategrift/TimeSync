-- ts_ui.text_input
-- Custom text input with multi-tap keyboard

local lvgl = require("lvgl")
local colors = require("ts_ui.colors")
local icons = require("ts_ui.icons")

local M = {}

-- Active input instance
local active_input = nil

-- Keyboard layouts
local LAYOUTS = {
    -- Uppercase
    {
        { "ABC", "DEF", "GHI" },
        { "JKL", "MNO", "PQR" },
        { "STUV", "WXYZ", ".,?!" },
    },
    -- Lowercase
    {
        { "abc", "def", "ghi" },
        { "jkl", "mno", "pqr" },
        { "stuv", "wxyz", ".,?!" },
    },
    -- Numbers
    {
        { "123", "456", "789" },
        { "+-*", "/=%", "#&@" },
        { "()[]", "{}<>", "." },
    },
    -- Symbols
    {
        { "!@#", "$%^", "&*()" },
        { "-_=", "+[]", "{}|" },
        { "\\:;", "\"'`", "~,." },
    },
}

-- Colors from shared module
local KEY_BG = colors.key
local MODIFIER_BG = colors.modifier

-- Timeout for multi-tap (milliseconds)
local TAP_TIMEOUT = 500

--- Show the text input modal
-- @param options table:
--   - title: string - Title to display
--   - value: string - Initial value
--   - on_save: function(key, type, value) - Called with the text value
--   - key: string - Setting key to pass to on_save
function M.show(options)
    options = options or {}
    local title = options.title or "Input"
    local init_value = options.value or ""
    local on_save = options.on_save
    local key = options.key or "text"
    
    -- Close any existing input
    if active_input then
        M.hide()
    end
    
    -- Current state
    local current_text = init_value
    local layout_index = 1
    local tap_key = nil
    local tap_index = 0
    local tap_timer = nil
    
    -- Full screen overlay
    local overlay = lvgl.Object({
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
    
    active_input = {
        overlay = overlay,
        timer = nil,
    }
    
    -- Title (smaller font)
    overlay:Label {
        text = title,
        text_color = colors.text,
        text_font = lvgl.BUILTIN_FONT.MONTSERRAT_12,
        align = {
            type = lvgl.ALIGN.TOP_MID,
            x_ofs = 0,
            y_ofs = 10,
        },
    }
    
    -- Text input display (smaller, higher up)
    local input_display = overlay:Object {
        w = 160,
        h = 28,
        bg_color = colors.card,
        radius = 5,
        outline_width = 0,
        border_width = 0,
        align = {
            type = lvgl.ALIGN.TOP_MID,
            x_ofs = 0,
            y_ofs = 28,
        },
    }
    input_display:clear_flag(lvgl.FLAG.SCROLLABLE)
    
    local text_label = input_display:Label {
        text = current_text,
        text_color = colors.text,
        text_font = lvgl.BUILTIN_FONT.MONTSERRAT_12,
        align = {
            type = lvgl.ALIGN.LEFT_MID,
            x_ofs = 8,
            y_ofs = 0,
        },
    }
    
    -- Button dimensions (compact for 240x240 round screen)
    -- Grid: 4 cols * 44px + 3 gaps * 3px = 185px, centered: (240-185)/2 = 28px
    local btn_w = 44
    local btn_h = 36
    local btn_gap = 3
    local grid_start_y = 62
    local grid_start_x = 28
    
    -- Key button references for updating
    local key_buttons = {}
    
    -- Function to update text display
    local function update_display()
        text_label.text = current_text
    end
    
    -- Function to reset tap state
    local function reset_tap()
        tap_key = nil
        tap_index = 0
        if active_input and active_input.timer then
            active_input.timer:delete()
            active_input.timer = nil
        end
    end
    
    -- Function to add character
    local function add_char(char)
        current_text = current_text .. char
        update_display()
    end
    
    -- Function to delete character
    local function delete_char()
        if #current_text > 0 then
            current_text = string.sub(current_text, 1, -2)
            update_display()
        end
        reset_tap()
    end
    
    -- Function to start tap timeout timer
    local function start_tap_timer()
        if active_input and active_input.timer then
            active_input.timer:delete()
        end
        active_input.timer = lvgl.Timer({
            period = TAP_TIMEOUT,
            repeat_count = 1,
            cb = function()
                reset_tap()
            end,
            paused = false,
        })
    end
    
    -- Function to handle key tap (multi-tap input)
    local function handle_key_tap(key_chars)
        if tap_key == key_chars and tap_index > 0 then
            -- Same key within timeout, cycle through characters
            tap_index = (tap_index % #key_chars) + 1
            -- Remove last char and add new one
            if #current_text > 0 then
                current_text = string.sub(current_text, 1, -2)
            end
            add_char(string.sub(key_chars, tap_index, tap_index))
        else
            -- New key or timeout expired
            tap_key = key_chars
            tap_index = 1
            add_char(string.sub(key_chars, 1, 1))
        end
        -- Restart timeout timer
        start_tap_timer()
    end
    
    -- Function to update keyboard layout
    local function update_keyboard()
        local layout = LAYOUTS[layout_index]
        for row = 1, 3 do
            for col = 1, 3 do
                local btn_data = key_buttons[row][col]
                if btn_data and layout[row] and layout[row][col] then
                    btn_data.label.text = layout[row][col]
                    btn_data.chars = layout[row][col]
                end
            end
        end
    end
    
    -- Create letter keys (3 cols x 3 rows)
    for row = 1, 3 do
        key_buttons[row] = {}
        for col = 1, 3 do
            local x = grid_start_x + (col - 1) * (btn_w + btn_gap)
            local y = grid_start_y + (row - 1) * (btn_h + btn_gap)
            local chars = LAYOUTS[1][row][col]
            
            local btn = overlay:Object {
                x = x,
                y = y,
                w = btn_w,
                h = btn_h,
                bg_color = KEY_BG,
                radius = 6,
                outline_width = 0,
                border_width = 0,
            }
            btn:clear_flag(lvgl.FLAG.SCROLLABLE)
            
            local lbl = btn:Label {
                text = chars,
                text_color = colors.text,
                text_font = lvgl.BUILTIN_FONT.MONTSERRAT_12,
                align = lvgl.ALIGN.CENTER,
            }
            
            key_buttons[row][col] = {
                btn = btn,
                label = lbl,
                chars = chars,
            }
            
            -- Capture row/col for closure
            local r, c = row, col
            btn:onClicked(function()
                local key_data = key_buttons[r][c]
                handle_key_tap(key_data.chars)
            end)
        end
    end
    
    -- Create modifier column (4th column)
    local mod_x = grid_start_x + 3 * (btn_w + btn_gap)
    
    -- Delete button
    local delete_btn = overlay:Object {
        x = mod_x,
        y = grid_start_y,
        w = btn_w,
        h = btn_h,
        bg_color = MODIFIER_BG,
        radius = 6,
        outline_width = 0,
        border_width = 0,
    }
    delete_btn:clear_flag(lvgl.FLAG.SCROLLABLE)
    delete_btn:Label {
        text = icons.backspace or "X",
        text_color = colors.text,
        align = lvgl.ALIGN.CENTER,
    }
    delete_btn:onClicked(function()
        delete_char()
    end)
    
    -- Mode switch button (arrow)
    local mode_btn = overlay:Object {
        x = mod_x,
        y = grid_start_y + btn_h + btn_gap,
        w = btn_w,
        h = btn_h,
        bg_color = MODIFIER_BG,
        radius = 6,
        outline_width = 0,
        border_width = 0,
    }
    mode_btn:clear_flag(lvgl.FLAG.SCROLLABLE)
    mode_btn:Label {
        text = icons.up or "^",
        text_color = colors.text,
        align = lvgl.ALIGN.CENTER,
    }
    mode_btn:onClicked(function()
        layout_index = (layout_index % #LAYOUTS) + 1
        update_keyboard()
        reset_tap()
    end)
    
    -- Confirm button (checkmark)
    local confirm_btn = overlay:Object {
        x = mod_x,
        y = grid_start_y + 2 * (btn_h + btn_gap),
        w = btn_w,
        h = btn_h,
        bg_color = colors.primary,
        radius = 6,
        outline_width = 0,
        border_width = 0,
    }
    confirm_btn:clear_flag(lvgl.FLAG.SCROLLABLE)
    confirm_btn:Label {
        text = icons.ok or "OK",
        text_color = colors.text,
        align = lvgl.ALIGN.CENTER,
    }
    confirm_btn:onClicked(function()
        reset_tap()
        M.hide()
        if on_save then
            on_save(key, "string", current_text)
        end
    end)
    
    -- Space bar (spans middle 2 columns at bottom)
    local space_y = grid_start_y + 3 * (btn_h + btn_gap)
    local space_w = btn_w * 2 + btn_gap
    local space_x = grid_start_x + btn_w + btn_gap  -- Start at 2nd column
    
    local space_btn = overlay:Object {
        x = space_x,
        y = space_y,
        w = space_w,
        h = btn_h,
        bg_color = colors.primary,
        radius = 6,
        outline_width = 0,
        border_width = 0,
    }
    space_btn:clear_flag(lvgl.FLAG.SCROLLABLE)
    space_btn:Label {
        text = "SPACE",
        text_color = colors.text,
        text_font = lvgl.BUILTIN_FONT.MONTSERRAT_12,
        align = lvgl.ALIGN.CENTER,
    }
    space_btn:onClicked(function()
        reset_tap()
        add_char(" ")
    end)
    
    return overlay
end

--- Hide the text input
function M.hide()
    if active_input then
        if active_input.timer then
            active_input.timer:delete()
        end
        active_input.overlay:delete()
        active_input = nil
    end
end

--- Check if input is visible
function M.is_visible()
    return active_input ~= nil
end

return M
