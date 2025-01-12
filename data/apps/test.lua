local lvgl = require("lvgl")

-- Define a table of numbers
local numbers = {3, 7, 1, 9, 4, 2}

-- Function to calculate the sum, mean, and standard deviation of a list of numbers
local function calculateStats(numTable)
    local sum = 0
    local n = #numTable

    -- Calculate sum
    for _, num in ipairs(numTable) do
        sum = sum + num
    end

    local mean = sum / n

    -- Calculate variance
    local variance = 0
    for _, num in ipairs(numTable) do
        variance = variance + (num - mean)^2
    end
    variance = variance / n

    local stdDev = math.sqrt(variance)

    return sum, mean, stdDev
end

-- Calculate stats for the numbers table
local sum, mean, stdDev = calculateStats(numbers)

-- Print the results
print("Numbers:", table.concat(numbers, ", "))
print("Sum:", sum)
print("Mean:", mean)
print("Standard Deviation:", stdDev)

-- Perform some complex math
local complexResult = math.sin(sum) * math.log(mean) + math.exp(stdDev)
print("Complex Math Result:", complexResult)

local root = lvgl.Object {
    w = lvgl.HOR_RES(),
    h = lvgl.VER_RES(),
    x = 0,
    y = 0,
    bg_opa = lvgl.OPA(0),
}

local button = root:Object {
    w = 60,
    h = 60,
    x = 0,
    y = 0,
    bg_color = "#888",
    bg_opa = lvgl.OPA(100),
    border_width = 0,
    radius = 10,
    pad_all = 20,
}

local button2 = root:Object {
    w = 40,
    h = 40,
    x = 40,
    y = 40,
    bg_color = "#994",
    bg_opa = lvgl.OPA(100),
    border_width = 0,
    radius = 10,
    pad_all = 20,
}
-- local shouldClose = false

local label = button2:Label {
    text = "Hey",
    text_color = "#333",
    align = lvgl.ALIGN.CENTER,
}
local i = 1
button2:onClicked(function()
    label.text = "Hey " .. i
    i = i + 1
end)

button:onClicked(function()
    root:delete()
    close()
end)

button:Label {
    text = "Hey",
    text_color = "#333",
    align = lvgl.ALIGN.CENTER,
}

-- Main loop that runs until close() is called
-- while not shouldClose do
--    -- do nothing
-- end

-- close()



