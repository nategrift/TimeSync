local lvgl = require("lvgl")

function showOptionScreen(parent)
    local option_bg = parent:Object {
        w = lvgl.HOR_RES(),
        h = lvgl.VER_RES(),
        bg_color = "#c00", -- red background
        bg_opa = lvgl.OPA(100),
        pad_all = 0,
        outline_width = 0,
        border_width = 0,
    }

    option_bg:Label {
        text = "Option Screen",
        text_color = "#fff",
        align = lvgl.ALIGN.CENTER,
        font = lvgl.BUILTIN_FONT.MONTSERRAT_42,
    }
end