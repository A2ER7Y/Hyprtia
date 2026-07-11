-- Hyprtia defaults for StratOS Hyprland Lua sessions.
-- This file is managed by hyprtia-hyprland-setup.

hl.config({
    input = {
        -- French AZERTY keyboard layout.
        kb_layout = "fr",
    },
})

hl.on("hyprland.start", function()
    hl.exec_cmd("hyprtia-hyprland-session")
end)

-- Always keep recovery one key away, including while Hyprtia Safe Mode is active.
hl.bind("SUPER + B", hl.dsp.exec_cmd("hyprtia-debug-report"))

if hl.plugin.confined_floats ~= nil then
    hl.window_rule({
        name = "hyprtia-confined-floats",
        match = { class = ".*" },
        ["confined-floats:confine"] = true,
    })
end

return {}
