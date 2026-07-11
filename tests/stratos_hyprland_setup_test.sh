#!/bin/sh

set -eu

setup_script=$1
session_script=$2
conf_resource=$3
lua_resource=$4
rule_resource=$5
test_root=$(mktemp -d)
trap 'rm -rf "$test_root"' EXIT HUP INT TERM

install -Dm755 "$setup_script" "$test_root/prefix/bin/hyprtia-hyprland-setup"
install -Dm755 "$session_script" "$test_root/prefix/bin/hyprtia-hyprland-session"
install -Dm644 "$conf_resource" "$test_root/prefix/share/hyprtia/hyprtia-hyprland.conf"
install -Dm644 "$lua_resource" "$test_root/prefix/share/hyprtia/hyprtia-hyprland.lua"
install -Dm644 "$rule_resource" "$test_root/prefix/share/hyprtia/hyprtia-confined-floats.conf"
mkdir -p "$test_root/stubs"

cat >"$test_root/stubs/hyprpm" <<'EOF'
#!/bin/sh
printf '%s\n' "$*" >>"$HYPRTIA_HYPRPM_TEST_LOG"
case "${1:-}" in
  list)
    if [ -f "$HYPRTIA_PLUGIN_ADDED" ]; then
      printf '%s\n' "Repository confined-floats"
    fi
    ;;
  add) touch "$HYPRTIA_PLUGIN_ADDED" ;;
esac
EOF
chmod 755 "$test_root/stubs/hyprpm"

cat >"$test_root/stubs/hyprctl" <<'EOF'
#!/bin/sh
printf '%s\n' "$*" >>"$HYPRTIA_HYPRCTL_TEST_LOG"
exit 1
EOF
chmod 755 "$test_root/stubs/hyprctl"

export HOME="$test_root/home"
export XDG_CONFIG_HOME="$test_root/config"
export XDG_STATE_HOME="$test_root/state"
export HYPRTIA_HYPRPM_TEST_LOG="$test_root/hyprpm.log"
export HYPRTIA_HYPRCTL_TEST_LOG="$test_root/hyprctl.log"
export HYPRTIA_PLUGIN_ADDED="$test_root/plugin-added"
export PATH="$test_root/stubs:/usr/bin:/bin"

"$test_root/prefix/bin/hyprtia-hyprland-setup" --auto

cmp "$conf_resource" "$XDG_CONFIG_HOME/hypr/hyprtia-stratos.conf"
cmp "$rule_resource" "$XDG_CONFIG_HOME/hypr/hyprtia-confined-floats.conf"
grep -q 'kb_layout = fr' "$XDG_CONFIG_HOME/hypr/hyprtia-stratos.conf"
grep -q 'exec-once = hyprtia-hyprland-session' "$XDG_CONFIG_HOME/hypr/hyprtia-stratos.conf"
grep -q '^windowrule {$' "$XDG_CONFIG_HOME/hypr/hyprtia-confined-floats.conf"
grep -q 'confined-floats:confine' "$XDG_CONFIG_HOME/hypr/hyprtia-confined-floats.conf"
! grep -q 'windowrulev2' "$XDG_CONFIG_HOME/hypr/hyprtia-confined-floats.conf"
grep -q 'source = .*hyprtia-stratos.conf' "$XDG_CONFIG_HOME/hypr/hyprland.conf"
grep -q "keyword source $XDG_CONFIG_HOME/hypr/hyprtia-confined-floats.conf" "$HYPRTIA_HYPRCTL_TEST_LOG"
test -f "$XDG_STATE_HOME/hyprtia/confined-floats.enabled"
printf '%s\n' \
  list \
  'add https://github.com/mennemann/hyprland-confined-floats' \
  'enable confined-floats' \
  reload | cmp - "$HYPRTIA_HYPRPM_TEST_LOG"

cp "$HYPRTIA_HYPRPM_TEST_LOG" "$test_root/hyprpm-first.log"
"$test_root/prefix/bin/hyprtia-hyprland-setup" --auto
cmp "$test_root/hyprpm-first.log" "$HYPRTIA_HYPRPM_TEST_LOG"
test "$(grep -c 'source = .*hyprtia-stratos.conf' "$XDG_CONFIG_HOME/hypr/hyprland.conf")" -eq 1

export XDG_CONFIG_HOME="$test_root/lua-config"
export XDG_STATE_HOME="$test_root/lua-state"
export HYPRTIA_HYPRLAND_CONFIG_MODE=lua
mkdir -p "$XDG_CONFIG_HOME/hypr"
printf '%s\n' '-- existing Lua configuration' >"$XDG_CONFIG_HOME/hypr/hyprland.lua"

"$test_root/prefix/bin/hyprtia-hyprland-setup" --config-only

cmp "$lua_resource" "$XDG_CONFIG_HOME/hypr/hyprtia-stratos.lua"
grep -q 'kb_layout = "fr"' "$XDG_CONFIG_HOME/hypr/hyprtia-stratos.lua"
grep -q 'confined_floats' "$XDG_CONFIG_HOME/hypr/hyprtia-stratos.lua"
grep -qF 'require("hyprtia-stratos")' "$XDG_CONFIG_HOME/hypr/hyprland.lua"
"$test_root/prefix/bin/hyprtia-hyprland-setup" --config-only
test "$(grep -cF 'require("hyprtia-stratos")' "$XDG_CONFIG_HOME/hypr/hyprland.lua")" -eq 1
