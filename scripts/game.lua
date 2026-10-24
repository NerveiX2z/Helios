-- Helios future game scripting example.
-- This script is NOT executed by the current C++ engine yet.
local game = {}
function game.on_start() print("Hello from Helios Lua!") end
function game.on_update(dt) -- Called after Lua runtime integration
end
return game
