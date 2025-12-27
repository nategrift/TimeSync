-- ts_ui.list
-- Reusable scrollable list component with configurable items

local lvgl = require("lvgl")
local colors = require("ts_ui.colors")
local icons = require("ts_ui.icons")

local M = {}

-- Default dimensions
local DEFAULT_CARD_HEIGHT = 60
local DEFAULT_CARD_RADIUS = 12

--- Create a scrollable list component
-- @param parent LVGL object - The parent container
-- @param options table:
--   - items: table - Array of item configurations:
--       - display_name: string - Primary text label
--       - value: string|function - Secondary text (or function returning string)
--       - icon: string - Optional icon on the left
--       - show_arrow: boolean - Show right arrow indicator (default: true)
--       - on_click: function(item, index) - Click handler
--       - data: any - Optional user data passed to callbacks
--   - card_height: number - Height of each card (default: 60)
--   - card_width: number - Width of cards (default: parent width - 80)
--   - card_spacing: number - Gap between cards (default: 10)
--   - padding_top: number - Top padding for the list (default: 60)
--   - padding_bottom: number - Bottom padding for the list (default: 60)
--   - padding_sides: number - Left/right padding for the list (default: 30)
-- @return table - List controller with methods: update_item, get_item, refresh
function M.create(parent, options)
    options = options or {}
    local items = options.items or {}
    local card_height = options.card_height or DEFAULT_CARD_HEIGHT
    local card_spacing = options.card_spacing or 10
    local padding_top = options.padding_top or 60
    local padding_bottom = options.padding_bottom or 60
    local padding_sides = options.padding_sides or 30
    
    -- Calculate card width
    local card_width = options.card_width or (lvgl.HOR_RES() - (padding_sides * 2) - 20)
    
    -- Create scrollable container
    local container = parent:Object {
        w = lvgl.HOR_RES(),
        h = lvgl.VER_RES(),
        bg_color = colors.background,
        bg_opa = lvgl.OPA(0),
        outline_width = 0,
        border_width = 0,
        pad_top = padding_top,
        pad_bottom = padding_bottom,
        pad_left = padding_sides,
        pad_right = padding_sides,
        pad_row = card_spacing,
        flex = {
            flex_direction = "column",
            flex_wrap = "nowrap",
            justify_content = "flex_start",
            align_items = "center",
            align_content = "center",
        },
        scrollbar_mode = lvgl.SCROLLBAR_MODE.OFF,
    }
    
    -- Store references to cards and their labels for updates
    local card_refs = {}
    
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
    
    -- Create a single card
    local function create_card(item, index)
        local has_icon = item.icon ~= nil
        local show_arrow = item.show_arrow ~= false  -- Default to true
        
        local card = container:Object {
            w = card_width,
            h = card_height,
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
        
        local label_x_offset = 0
        
        -- Optional left icon
        if has_icon then
            card:Label {
                text = item.icon,
                text_color = colors.primary,
                align = {
                    type = lvgl.ALIGN.LEFT_MID,
                    x_ofs = 0,
                    y_ofs = 0,
                },
            }
            label_x_offset = 25
        end
        
        -- Display name
        local value_text = get_value_text(item)
        local has_value = value_text ~= ""
        
        local name_label = card:Label {
            text = item.display_name or "",
            text_color = colors.text,
            align = {
                type = lvgl.ALIGN.LEFT_MID,
                x_ofs = label_x_offset,
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
                    x_ofs = label_x_offset,
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
                item.on_click(item, index)
            end)
        end
        
        return {
            card = card,
            name_label = name_label,
            value_label = value_label,
            item = item,
            index = index,
        }
    end
    
    -- Build all cards
    for i, item in ipairs(items) do
        card_refs[i] = create_card(item, i)
    end
    
    -- List controller
    local controller = {
        container = container,
        cards = card_refs,
    }
    
    --- Update a specific item's value display
    -- @param index number - 1-based index of the item
    -- @param new_value string|nil - New value to display (nil to refresh from item.value)
    function controller:update_item(index, new_value)
        local ref = card_refs[index]
        if not ref then return end
        
        if new_value ~= nil then
            ref.item.value = new_value
        end
        
        if ref.value_label then
            ref.value_label.text = get_value_text(ref.item)
        end
    end
    
    --- Get a card reference by index
    -- @param index number - 1-based index
    -- @return table - Card reference with card, name_label, value_label, item
    function controller:get_item(index)
        return card_refs[index]
    end
    
    --- Refresh all items' value displays
    function controller:refresh()
        for i, ref in ipairs(card_refs) do
            if ref.value_label then
                ref.value_label.text = get_value_text(ref.item)
            end
        end
    end
    
    --- Delete the list
    function controller:delete()
        container:delete()
    end
    
    return controller
end

return M

