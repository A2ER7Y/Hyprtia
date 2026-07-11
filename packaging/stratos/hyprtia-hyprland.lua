-- Hyprtia defaults for StratOS Hyprland Lua sessions.
-- This file is managed by hyprtia-hyprland-setup.

hl.config({
    input = {
        -- French AZERTY keyboard layout.
        kb_layout = "fr",
    },
    ["exec-once"] = {
        "sh -c 'hyprpm reload && hyprctl reload'",
    },
})

if hl.plugin.confined_floats ~= nil then
    hl.window_rule({
        name = "hyprtia-confined-floats",
        match = { class = ".*" },
        ["confined-floats:confine"] = true,
    })
end

return {}
