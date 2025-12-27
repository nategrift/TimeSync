-- ts_ui.sub_pick
-- Submenu/list picker for nested settings menus

local lvgl = require("lvgl")
local colors = require("ts_ui.colors")
local icons = require("ts_ui.icons")

local M = {}

-- Active picker instance
local active_picker = nil

-- Default dimensions
local DEFAULT_CARD_HEIGHT = 60
local DEFAULT_CARD_RADIUS = 12

--- Show the submenu picker modal
-- @param options table:
--   - title: string - Title to display at top
--   - items: table - Array of item configurations:
--       - display_name: string - Primary text label
--       - value: string|function - Secondary text (or function returning string)
--       - show_arrow: boolean - Show right arrow indicator (default: false for info items)
--       - on_click: function(item, index) - Click handler (optional)
--   - on_back: function() - Called when back is pressed (optional)
function M.show(options)
    options = options or {}
    local title = options.title or "Menu"
    local items = options.items or {}
    local on_back = options.on_back
    
    -- Close any existing picker
    if active_picker then
        M.hide()
    end
    
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
    
    active_picker = {
        overlay = overlay,
        card_refs = {},
    }
    
    -- Title at top
    overlay:Label {
        text = title,
        text_color = colors.text,
        text_font = lvgl.BUILTIN_FONT.MONTSERRAT_16,
        align = {
            type = lvgl.ALIGN.TOP_MID,
            x_ofs = 0,
            y_ofs = 15,
        },
    }
    
    -- Scrollable container for items
    local container = overlay:Object {
        w = lvgl.HOR_RES() - 40,
        h = lvgl.VER_RES() - 100,
        bg_color = colors.background,
        bg_opa = lvgl.OPA(0),
        outline_width = 0,
        border_width = 0,
        pad_top = 10,
        pad_bottom = 10,
        pad_left = 0,
        pad_right = 0,
        pad_row = 8,
        flex = {
            flex_direction = "column",
            flex_wrap = "nowrap",
            justify_content = "flex_start",
            align_items = "center",
            align_content = "center",
        },
        scrollbar_mode = lvgl.SCROLLBAR_MODE.OFF,
        align = {
            type = lvgl.ALIGN.TOP_MID,
            x_ofs = 0,
            y_ofs = 45,
        },
    }
    
    -- Get value text from item
    local function get_value_text(item)
        if item.value == nil then
            return ""
        elseif type(item.value) == "function" then
            return tostring(item.value(item))
        else
            return tostring(item.value)
        end
    end
    
    -- Create cards for each item
    local card_width = lvgl.HOR_RES() - 60
    
    for i, item in ipairs(items) do
        local show_arrow = item.show_arrow or (item.on_click ~= nil)
        
        local card = container:Object {
            w = card_width,
            h = DEFAULT_CARD_HEIGHT,
            bg_color = colors.card,
            radius = DEFAULT_CARD_RADIUS,
            outline_width = 0,
            border_width = 0,
            pad_left = 15,
            pad_right = 15,
            pad_top = 0,
            pad_bottom = 0,
        }
        card:clear_flag(lvgl.FLAG.SCROLLABLE)
        
        -- Display name
        local value_text = get_value_text(item)
        local has_value = value_text ~= ""
        
        local name_label = card:Label {
            text = item.display_name or "",
            text_color = colors.text,
            align = {
                type = lvgl.ALIGN.LEFT_MID,
                x_ofs = 0,
                y_ofs = has_value and -10 or 0,
            },
        }
        
        -- Value label (below display name)
        local value_label = nil
        if has_value then
            value_label = card:Label {
                text = value_text,
                text_color = colors.secondary_text,
                align = {
                    type = lvgl.ALIGN.LEFT_MID,
                    x_ofs = 0,
                    y_ofs = 10,
                },
            }
        end
        
        -- Right arrow indicator
        if show_arrow then
            card:Label {
                text = icons.right,
                text_color = colors.secondary_text,
                align = {
                    type = lvgl.ALIGN.RIGHT_MID,
                    x_ofs = 0,
                    y_ofs = 0,
                },
            }
        end
        
        -- Click handler
        if item.on_click then
            card:onClicked(function()
                item.on_click(item, i)
            end)
        end
        
        active_picker.card_refs[i] = {
            card = card,
            name_label = name_label,
            value_label = value_label,
            item = item,
        }
    end
    
    -- Back button at bottom
    local back_btn = overlay:Object {
        w = 100,
        h = 36,
        bg_color = colors.card,
        radius = 10,
        outline_width = 0,
        border_width = 0,
        align = {
            type = lvgl.ALIGN.BOTTOM_MID,
            x_ofs = 0,
            y_ofs = -15,
        },
    }
    back_btn:clear_flag(lvgl.FLAG.SCROLLABLE)
    
    back_btn:Label {
        text = "Back",
        text_color = colors.text,
        text_font = lvgl.BUILTIN_FONT.MONTSERRAT_12,
        align = lvgl.ALIGN.CENTER,
    }
    
    back_btn:onClicked(function()
        M.hide()
        if on_back then
            on_back()
        end
    end)
    
    
    return overlay
end

--- Hide the submenu picker
function M.hide()
    if active_picker then
        active_picker.overlay:delete()
        active_picker = nil
    end
end

--- Check if picker is visible
function M.is_visible()
    return active_picker ~= nil
end

--- Refresh a specific item's value display
function M.update_item(index)
    if not active_picker or not active_picker.card_refs[index] then
        return
    end
    
    local ref = active_picker.card_refs[index]
    if ref.value_label and ref.item.value then
        local value_text
        if type(ref.item.value) == "function" then
            value_text = tostring(ref.item.value(ref.item))
        else
            value_text = tostring(ref.item.value)
        end
        ref.value_label.text = value_text
    end
end

--- Refresh all items' value displays
function M.refresh_all()
    if not active_picker then return end
    for i, _ in ipairs(active_picker.card_refs) do
        M.update_item(i)
    end
end

return M

