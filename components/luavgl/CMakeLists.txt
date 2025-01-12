idf_component_register(SRCS
    anim.c
    constants.c
    disp.c
    event.c
    font.c
    fs.c
    group.c
    indev.c
    luavgl.c
    palette.c
    rotable.c
    style.c
    timer.c
    util.c
    widgets/button.c
    widgets/calendar.c
    widgets/checkbox.c
    widgets/dropdown.c
    widgets/img.c
    widgets/keyboard.c
    widgets/label.c
    widgets/led.c
    widgets/list.c
    widgets/roller.c
    widgets/textarea.c
    widgets/widgets.c
    obj.c
    # Add more files as needed
    INCLUDE_DIRS "."
    REQUIRES "lua lvgl"
)
