local lvgl = require("lvgl")
local ts_ui = require("ts_ui")
local colors = ts_ui.colors

-- Calculator app accent color
local CALC_PRIMARY = colors.primary
-- Active operator color (darker)
local OP_ACTIVE = "#5A6A78"

LVGL_lock()

-- Calculator state
local current_value = "0"
local stored_value = nil
local current_op = nil
local just_calculated = false

-- Operator button references for visual feedback
local op_buttons = {}

-- Root container
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

-- Format number with commas
local function format_number(num_str)
    -- Handle negative numbers
    local negative = false
    if string.sub(num_str, 1, 1) == "-" then
        negative = true
        num_str = string.sub(num_str, 2)
    end
    
    -- Split by decimal point
    local int_part, dec_part = string.match(num_str, "([^.]*)%.?(.*)")
    
    -- Add commas to integer part
    local formatted = ""
    local len = #int_part
    for i = 1, len do
        formatted = formatted .. string.sub(int_part, i, i)
        local remaining = len - i
        if remaining > 0 and remaining % 3 == 0 then
            formatted = formatted .. ","
        end
    end
    
    -- Add decimal part back
    if dec_part and #dec_part > 0 then
        formatted = formatted .. "." .. dec_part
    end
    
    -- Add negative sign back
    if negative then
        formatted = "-" .. formatted
    end
    
    return formatted
end

-- Display (182px wide to match button grid)
local display = root:Object {
    w = 162,
    h = 30,
    bg_color = colors.card,
    radius = 8,
    outline_width = 0,
    border_width = 0,
    align = {
        type = lvgl.ALIGN.TOP_MID,
        x_ofs = 0,
        y_ofs = 28,
    },
}
display:clear_flag(lvgl.FLAG.SCROLLABLE)

local display_label = display:Label {
    text = format_number(current_value),
    text_color = colors.text,
    text_font = lvgl.BUILTIN_FONT.MONTSERRAT_22,
    align = {
        type = lvgl.ALIGN.RIGHT_MID,
        x_ofs = -10,
        y_ofs = 0,
    },
}

-- Update display
local function update_display()
    -- Truncate if too long
    local display_text = current_value
    if #display_text > 10 then
        display_text = string.sub(display_text, 1, 10)
    end
    display_label.text = format_number(display_text)
end

-- Update operator button visual states
local function update_op_buttons()
    for op, btn in pairs(op_buttons) do
        if op == current_op then
            btn.bg_color = OP_ACTIVE
        else
            btn.bg_color = colors.modifier
        end
    end
end

-- Perform calculation
local function calculate()
    if stored_value and current_op then
        local a = tonumber(stored_value)
        local b = tonumber(current_value)
        local result = 0
        
        if current_op == "+" then
            result = a + b
        elseif current_op == "-" then
            result = a - b
        elseif current_op == "x" then
            result = a * b
        elseif current_op == "/" then
            if b ~= 0 then
                result = a / b
            else
                current_value = "Error"
                update_display()
                stored_value = nil
                current_op = nil
                update_op_buttons()
                return
            end
        end
        
        -- Format result (remove trailing zeros for decimals)
        if result == math.floor(result) then
            current_value = tostring(math.floor(result))
        else
            current_value = string.format("%.6f", result)
            -- Remove trailing zeros
            current_value = string.gsub(current_value, "%.?0+$", "")
        end
        
        stored_value = nil
        current_op = nil
        just_calculated = true
        update_display()
        update_op_buttons()
    end
end

-- Handle number press
local function press_number(num)
    if just_calculated or current_value == "0" or current_value == "Error" then
        current_value = num
        just_calculated = false
    else
        if #current_value < 10 then
            current_value = current_value .. num
        end
    end
    update_display()
end

-- Handle decimal press
local function press_decimal()
    if just_calculated then
        current_value = "0."
        just_calculated = false
    elseif not string.find(current_value, "%.") then
        current_value = current_value .. "."
    end
    update_display()
end

-- Handle operator press
local function press_operator(op)
    if stored_value and current_op and not just_calculated then
        calculate()
    end
    stored_value = current_value
    current_op = op
    just_calculated = true
    update_op_buttons()
end

-- Handle clear
local function press_clear()
    current_value = "0"
    stored_value = nil
    current_op = nil
    just_calculated = false
    update_display()
    update_op_buttons()
end

-- Button layout
-- Grid: 5 cols x 3 rows, then 2 buttons at bottom
-- 5 buttons * 30px + 4 gaps * 8px = 150 + 32 = 182px
-- Start X = (240 - 182) / 2 = 29px
local btn_w = 30
local btn_h = 30
local btn_gap = 8
local grid_start_y = 70
local grid_start_x = 29

-- Button definitions for 3 rows
local buttons = {
    { "1", "2", "3", "+", "0" },
    { "4", "5", "6", "-", "." },
    { "7", "8", "9", "x", "/" },
}

-- Operators list
local operators = { ["+"] = true, ["-"] = true, ["x"] = true, ["/"] = true }

-- Create main grid buttons
for row = 1, 3 do
    for col = 1, 5 do
        local x = grid_start_x + (col - 1) * (btn_w + btn_gap)
        local y = grid_start_y + (row - 1) * (btn_h + btn_gap)
        local label = buttons[row][col]
        
        -- Use modifier color for operators, key color for numbers/decimal
        local bg_color = colors.key
        if operators[label] then
            bg_color = colors.modifier
        end
        
        local btn = root:Object {
            x = x,
            y = y,
            w = btn_w,
            h = btn_h,
            bg_color = bg_color,
            radius = 8,
            outline_width = 0,
            border_width = 0,
        }
        btn:clear_flag(lvgl.FLAG.SCROLLABLE)
        
        btn:Label {
            text = label,
            text_color = colors.text,
            text_font = lvgl.BUILTIN_FONT.MONTSERRAT_14,
            align = lvgl.ALIGN.CENTER,
        }
        
        -- Store operator button references
        if operators[label] then
            op_buttons[label] = btn
        end
        
        -- Capture label for closure
        local btn_label = label
        btn:onClicked(function()
            if btn_label >= "0" and btn_label <= "9" then
                press_number(btn_label)
            elseif btn_label == "." then
                press_decimal()
            elseif operators[btn_label] then
                press_operator(btn_label)
            end
        end)
    end
end

-- Bottom row: C (clear) and = (equals, spanning 2 columns)
local bottom_y = grid_start_y + 3 * (btn_h + btn_gap)

-- Clear button
local clear_btn = root:Object {
    x = grid_start_x + 1 * (btn_w + btn_gap),
    y = bottom_y,
    w = btn_w,
    h = btn_h,
    bg_color = colors.modifier,
    radius = 8,
    outline_width = 0,
    border_width = 0,
}
clear_btn:clear_flag(lvgl.FLAG.SCROLLABLE)
clear_btn:Label {
    text = "C",
    text_color = colors.text,
    text_font = lvgl.BUILTIN_FONT.MONTSERRAT_14,
    align = lvgl.ALIGN.CENTER,
}
clear_btn:onClicked(function()
    press_clear()
end)

-- Equals button (spans 2 columns, calculator accent color)
local equals_btn = root:Object {
    x = grid_start_x + 2 * (btn_w + btn_gap),
    y = bottom_y,
    w = btn_w * 2 + btn_gap,
    h = btn_h,
    bg_color = CALC_PRIMARY,
    radius = 8,
    outline_width = 0,
    border_width = 0,
}
equals_btn:clear_flag(lvgl.FLAG.SCROLLABLE)
equals_btn:Label {
    text = "=",
    text_color = colors.text,
    text_font = lvgl.BUILTIN_FONT.MONTSERRAT_14,
    align = lvgl.ALIGN.CENTER,
}
equals_btn:onClicked(function()
    calculate()
end)

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
    root:delete()
end
