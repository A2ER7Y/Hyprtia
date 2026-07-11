#!/bin/sh

set -eu

setup_script=$1
session_script=$2
conf_resource=$3
lua_resource=$4
rule_resource=$5
strat_runner=$6
test_root=$(mktemp -d)
trap 'rm -rf "$test_root"' EXIT HUP INT TERM

install -Dm755 "$setup_script" "$test_root/prefix/bin/hyprtia-hyprland-setup"
install -Dm755 "$session_script" "$test_root/prefix/bin/hyprtia-hyprland-session"
install -Dm755 "$strat_runner" "$test_root/prefix/bin/hyprtia-strat"
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
case "$*" in
  "dispatch hl.dsp.no_op()") printf '%s\n' ok ;;
esac
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

cmp "$lua_resource" "$XDG_CONFIG_HOME/hypr/hyprtia-stratos.lua"
cmp "$rule_resource" "$XDG_CONFIG_HOME/hypr/hyprtia-confined-floats.conf"
grep -q 'kb_layout = "fr"' "$XDG_CONFIG_HOME/hypr/hyprtia-stratos.lua"
grep -q 'hl.on("hyprland.start"' "$XDG_CONFIG_HOME/hypr/hyprtia-stratos.lua"
grep -q 'hl.exec_cmd("hyprtia-hyprland-session")' "$XDG_CONFIG_HOME/hypr/hyprtia-stratos.lua"
grep -qF 'hl.bind("SUPER + B", hl.dsp.exec_cmd("hyprtia-debug-report"))' \
  "$XDG_CONFIG_HOME/hypr/hyprtia-stratos.lua"
! grep -q 'exec-once' "$XDG_CONFIG_HOME/hypr/hyprtia-stratos.lua"
grep -qF 'require("hyprtia-stratos")' "$XDG_CONFIG_HOME/hypr/hyprland.lua"
test ! -e "$XDG_CONFIG_HOME/hypr/hyprland.conf"
grep -q '^windowrule {$' "$XDG_CONFIG_HOME/hypr/hyprtia-confined-floats.conf"
grep -q 'confined-floats:confine' "$XDG_CONFIG_HOME/hypr/hyprtia-confined-floats.conf"
! grep -q 'windowrulev2' "$XDG_CONFIG_HOME/hypr/hyprtia-confined-floats.conf"
grep -qF 'dispatch hl.dsp.no_op()' "$HYPRTIA_HYPRCTL_TEST_LOG"
grep -qx reload "$HYPRTIA_HYPRCTL_TEST_LOG"
test -f "$XDG_STATE_HOME/hyprtia/confined-floats.enabled"
printf '%s\n' \
  list \
  'add https://github.com/mennemann/hyprland-confined-floats' \
  'enable confined-floats' \
  reload | cmp - "$HYPRTIA_HYPRPM_TEST_LOG"

cp "$HYPRTIA_HYPRPM_TEST_LOG" "$test_root/hyprpm-first.log"
"$test_root/prefix/bin/hyprtia-hyprland-setup" --auto
cmp "$test_root/hyprpm-first.log" "$HYPRTIA_HYPRPM_TEST_LOG"
test "$(grep -cF 'require("hyprtia-stratos")' "$XDG_CONFIG_HOME/hypr/hyprland.lua")" -eq 1

export XDG_CONFIG_HOME="$test_root/legacy-config"
export XDG_STATE_HOME="$test_root/legacy-state"
mkdir -p "$XDG_CONFIG_HOME/hypr"
printf '%s\n' '# existing legacy configuration' >"$XDG_CONFIG_HOME/hypr/hyprland.conf"

"$test_root/prefix/bin/hyprtia-hyprland-setup" --config-only

cmp "$conf_resource" "$XDG_CONFIG_HOME/hypr/hyprtia-stratos.conf"
grep -q 'kb_layout = fr' "$XDG_CONFIG_HOME/hypr/hyprtia-stratos.conf"
grep -q 'source = .*hyprtia-stratos.conf' "$XDG_CONFIG_HOME/hypr/hyprland.conf"
test ! -e "$XDG_CONFIG_HOME/hypr/hyprland.lua"
"$test_root/prefix/bin/hyprtia-hyprland-setup" --config-only
test "$(grep -c 'source = .*hyprtia-stratos.conf' "$XDG_CONFIG_HOME/hypr/hyprland.conf")" -eq 1
