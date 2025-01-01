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
