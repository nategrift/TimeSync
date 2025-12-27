local lvgl = require("lvgl")
local ts_ui = require("ts_ui")
local colors = ts_ui.colors

-- Game constants
local SCREEN_SIZE = 240
local CENTER = SCREEN_SIZE / 2
local ARENA_RADIUS = 105
local BALL_RADIUS = 5
local BALL_SPEED = 2.5
local AI_SPEED = 0.03
local WIN_SCORE = 5
local PADDLE_HALF_ARC = 0.4  -- ~23 degrees in radians

-- Custom atan2 since not available in embedded Lua
local function atan2(y, x)
    if x > 0 then
        return math.atan(y / x)
    elseif x < 0 and y >= 0 then
        return math.atan(y / x) + math.pi
    elseif x < 0 and y < 0 then
        return math.atan(y / x) - math.pi
    elseif x == 0 and y > 0 then
        return math.pi / 2
    elseif x == 0 and y < 0 then
        return -math.pi / 2
    else
        return 0
    end
end

-- Game state
local game_state = "menu"
local player_score = 0
local enemy_score = 0

-- Ball state
local ball_x = CENTER
local ball_y = CENTER
local ball_vx = 0
local ball_vy = 0

-- Paddle angles (radians)
local player_angle = math.pi / 2
local enemy_angle = 3 * math.pi / 2

-- Touch state
local last_touch_x = 0
local touch_active = false

-- UI elements (created/destroyed per screen)
local root = nil
local game_timer = nil
local ball = nil
local player_paddle = nil
local enemy_paddle = nil
local score_label = nil

LVGL_lock()

root = lvgl.Object {
    w = SCREEN_SIZE,
    h = SCREEN_SIZE,
    bg_color = colors.background,
    bg_opa = lvgl.OPA(100),
    pad_all = 0,
    border_width = 0,
    scrollbar_mode = lvgl.SCROLLBAR_MODE.OFF,
}

-- Forward declarations
local show_menu, show_game, show_gameover

-- ==================== MENU SCREEN ====================
local function create_menu()
    root:Label {
        text = "ROUND",
        text_color = "#FFFFFF",
        text_font = lvgl.BUILTIN_FONT.MONTSERRAT_30,
        align = { type = lvgl.ALIGN.CENTER, x_ofs = 0, y_ofs = -50 },
    }

    root:Label {
        text = "PONG",
        text_color = colors.primary,
        text_font = lvgl.BUILTIN_FONT.MONTSERRAT_30,
        align = { type = lvgl.ALIGN.CENTER, x_ofs = 0, y_ofs = -10 },
    }

    local play_btn = root:Object {
        w = 100, h = 36, radius = 8,
        bg_color = colors.card,
        bg_opa = lvgl.OPA(100),
        border_width = 0,
        align = { type = lvgl.ALIGN.CENTER, x_ofs = 0, y_ofs = 50 },
    }

    play_btn:Label {
        text = "Play",
        text_color = "#FFFFFF",
        text_font = lvgl.BUILTIN_FONT.MONTSERRAT_16,
        align = lvgl.ALIGN.CENTER,
    }

    play_btn:onevent(lvgl.EVENT.CLICKED, function()
        show_game()
    end)

    -- Swipe to exit
    root:onevent(lvgl.EVENT.GESTURE, function(obj, code)
        local indev = lvgl.indev.get_act()
        if indev then
            local dir = indev:get_gesture_dir()
            if dir == lvgl.DIR.LEFT or dir == lvgl.DIR.RIGHT then
                ts_ui.app_selector.show()
            end
        end
    end)
end

-- ==================== GAME SCREEN ====================
local function reset_ball(toward_player)
    ball_x = CENTER
    ball_y = CENTER
    local angle = (math.random() - 0.5) * math.pi / 3
    local direction = toward_player and 1 or -1
    ball_vx = math.sin(angle) * BALL_SPEED
    ball_vy = direction * math.cos(angle) * BALL_SPEED
end

local function update_game()
    if game_state ~= "playing" then return end

    ball_x = ball_x + ball_vx
    ball_y = ball_y + ball_vy

    local dx = ball_x - CENTER
    local dy = ball_y - CENTER
    local dist = math.sqrt(dx * dx + dy * dy)

    if dist >= ARENA_RADIUS - BALL_RADIUS then
        local ball_angle = atan2(dy, dx)
        if ball_angle < 0 then ball_angle = ball_angle + 2 * math.pi end

        local in_player_zone = ball_angle > 0.2 and ball_angle < math.pi - 0.2
        local in_enemy_zone = ball_angle > math.pi + 0.2 and ball_angle < 2 * math.pi - 0.2

        local paddle_angle = in_player_zone and player_angle or enemy_angle
        local angle_diff = math.abs(ball_angle - paddle_angle)
        if angle_diff > math.pi then angle_diff = 2 * math.pi - angle_diff end

        if (in_player_zone or in_enemy_zone) and angle_diff < PADDLE_HALF_ARC then
            -- Hit paddle
            local normal_x = -dx / dist
            local normal_y = -dy / dist
            local dot = ball_vx * normal_x + ball_vy * normal_y
            ball_vx = ball_vx - 2 * dot * normal_x
            ball_vy = ball_vy - 2 * dot * normal_y

            local speed = math.sqrt(ball_vx * ball_vx + ball_vy * ball_vy)
            if speed < BALL_SPEED * 2 then
                ball_vx = ball_vx * 1.05
                ball_vy = ball_vy * 1.05
            end

            ball_x = CENTER + (ARENA_RADIUS - BALL_RADIUS - 2) * math.cos(ball_angle)
            ball_y = CENTER + (ARENA_RADIUS - BALL_RADIUS - 2) * math.sin(ball_angle)
        elseif in_player_zone then
            enemy_score = enemy_score + 1
            if score_label then score_label.text = player_score .. " - " .. enemy_score end
            if enemy_score >= WIN_SCORE then
                show_gameover(false)
                return
            end
            reset_ball(false)
        elseif in_enemy_zone then
            player_score = player_score + 1
            if score_label then score_label.text = player_score .. " - " .. enemy_score end
            if player_score >= WIN_SCORE then
                show_gameover(true)
                return
            end
            reset_ball(true)
        else
            -- Side walls
            local normal_x = -dx / dist
            local normal_y = -dy / dist
            local dot = ball_vx * normal_x + ball_vy * normal_y
            ball_vx = ball_vx - 2 * dot * normal_x
            ball_vy = ball_vy - 2 * dot * normal_y
            ball_x = CENTER + (ARENA_RADIUS - BALL_RADIUS - 1) * math.cos(ball_angle)
            ball_y = CENTER + (ARENA_RADIUS - BALL_RADIUS - 1) * math.sin(ball_angle)
        end
    end

    -- AI
    if ball_vy < 0 then
        local target_angle = atan2(ball_y - CENTER, ball_x - CENTER)
        if target_angle < 0 then target_angle = target_angle + 2 * math.pi end
        if target_angle > math.pi then
            local diff = target_angle - enemy_angle
            if diff > math.pi then diff = diff - 2 * math.pi end
            if diff < -math.pi then diff = diff + 2 * math.pi end
            if math.abs(diff) > 0.02 then
                enemy_angle = enemy_angle + (diff > 0 and AI_SPEED or -AI_SPEED)
            end
        end
    end
    if enemy_angle < math.pi + 0.2 then enemy_angle = math.pi + 0.2 end
    if enemy_angle > 2 * math.pi - 0.2 then enemy_angle = 2 * math.pi - 0.2 end

    -- Update visuals
    if ball then
        ball.x = math.floor(ball_x - BALL_RADIUS)
        ball.y = math.floor(ball_y - BALL_RADIUS)
    end
    if player_paddle then
        local px = CENTER + ARENA_RADIUS * math.cos(player_angle) - 20
        local py = CENTER + ARENA_RADIUS * math.sin(player_angle) - 3
        player_paddle.x = math.floor(px)
        player_paddle.y = math.floor(py)
    end
    if enemy_paddle then
        local ex = CENTER + ARENA_RADIUS * math.cos(enemy_angle) - 20
        local ey = CENTER + ARENA_RADIUS * math.sin(enemy_angle) - 3
        enemy_paddle.x = math.floor(ex)
        enemy_paddle.y = math.floor(ey)
    end
end

local function create_game()
    player_score = 0
    enemy_score = 0
    player_angle = math.pi / 2
    enemy_angle = 3 * math.pi / 2
    touch_active = false

    score_label = root:Label {
        text = "0 - 0",
        text_color = "#666666",
        text_font = lvgl.BUILTIN_FONT.MONTSERRAT_14,
        align = lvgl.ALIGN.CENTER,
    }

    ball = root:Object {
        w = BALL_RADIUS * 2, h = BALL_RADIUS * 2,
        radius = BALL_RADIUS,
        bg_color = "#FFFFFF",
        bg_opa = lvgl.OPA(100),
        border_width = 0,
        x = CENTER - BALL_RADIUS,
        y = CENTER - BALL_RADIUS,
    }

    player_paddle = root:Object {
        w = 40, h = 6, radius = 3,
        bg_color = "#FFFFFF",
        bg_opa = lvgl.OPA(100),
        border_width = 0,
        x = CENTER - 20,
        y = CENTER + ARENA_RADIUS - 3,
    }

    enemy_paddle = root:Object {
        w = 40, h = 6, radius = 3,
        bg_color = colors.primary,
        bg_opa = lvgl.OPA(100),
        border_width = 0,
        x = CENTER - 20,
        y = CENTER - ARENA_RADIUS - 3,
    }

    -- Side markers
    root:Object {
        w = 6, h = 20, radius = 3,
        bg_color = "#444444",
        bg_opa = lvgl.OPA(100),
        border_width = 0,
        x = CENTER - ARENA_RADIUS - 3,
        y = CENTER - 10,
    }
    root:Object {
        w = 6, h = 20, radius = 3,
        bg_color = "#444444",
        bg_opa = lvgl.OPA(100),
        border_width = 0,
        x = CENTER + ARENA_RADIUS - 3,
        y = CENTER - 10,
    }

    reset_ball(true)

    root:onevent(lvgl.EVENT.PRESSING, function(obj, code)
        if game_state ~= "playing" then return end
        local indev = lvgl.indev.get_act()
        if indev then
            local px, py = indev:get_point()
            if px then
                if touch_active then
                    local delta = (px - last_touch_x) * 0.02
                    player_angle = player_angle + delta
                    if player_angle < 0.2 then player_angle = 0.2 end
                    if player_angle > math.pi - 0.2 then player_angle = math.pi - 0.2 end
                end
                last_touch_x = px
                touch_active = true
            end
        end
    end)

    root:onevent(lvgl.EVENT.RELEASED, function()
        touch_active = false
    end)

    game_timer = lvgl.Timer {
        period = 33,
        cb = function() update_game() end,
    }
end

-- ==================== GAME OVER SCREEN ====================
local function create_gameover(player_won)
    root:Label {
        text = "GAME OVER",
        text_color = "#FFFFFF",
        text_font = lvgl.BUILTIN_FONT.MONTSERRAT_22,
        align = { type = lvgl.ALIGN.CENTER, x_ofs = 0, y_ofs = -40 },
    }

    root:Label {
        text = player_won and "You Win!" or "You Lose!",
        text_color = player_won and colors.primary or "#FF5555",
        text_font = lvgl.BUILTIN_FONT.MONTSERRAT_16,
        align = { type = lvgl.ALIGN.CENTER, x_ofs = 0, y_ofs = 0 },
    }

    local restart_btn = root:Object {
        w = 100, h = 36, radius = 8,
        bg_color = colors.card,
        bg_opa = lvgl.OPA(100),
        border_width = 0,
        align = { type = lvgl.ALIGN.CENTER, x_ofs = 0, y_ofs = 50 },
    }

    restart_btn:Label {
        text = "Play Again",
        text_color = "#FFFFFF",
        text_font = lvgl.BUILTIN_FONT.MONTSERRAT_14,
        align = lvgl.ALIGN.CENTER,
    }

    restart_btn:onevent(lvgl.EVENT.CLICKED, function()
        show_game()
    end)
end

-- ==================== SCREEN TRANSITIONS ====================
local function clear_root()
    if game_timer then
        game_timer:delete()
        game_timer = nil
    end
    ball = nil
    player_paddle = nil
    enemy_paddle = nil
    score_label = nil
    root:clean()
end

show_menu = function()
    clear_root()
    game_state = "menu"
    create_menu()
end

show_game = function()
    clear_root()
    game_state = "playing"
    create_game()
end

show_gameover = function(player_won)
    clear_root()
    game_state = "game_over"
    create_gameover(player_won)
end

-- Start with menu
show_menu()

LVGL_unlock()

OnClose = function()
    if game_timer then
        game_timer:delete()
    end
    if root then
        root:delete()
    end
end
