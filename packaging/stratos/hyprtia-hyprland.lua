-- Hyprtia defaults for StratOS Hyprland Lua sessions.
-- This file is managed by hyprtia-hyprland-setup.

hl.config({
    input = {
        -- French AZERTY keyboard layout.
        kb_layout = "fr",
    },
    ["exec-once"] = {
        "hyprpm reload",
    },
})

if hl.plugin.confined_floats ~= nil then
    hl.window_rule({
        match = { class = ".*" },
        ["confined-floats:confine"] = true,
    })
end

return {}
