#!/bin/sh

set -eu

transition=$1
test_root=$(mktemp -d)
trap 'rm -rf "$test_root"' EXIT HUP INT TERM

install -Dm755 "$transition" "$test_root/prefix/bin/hyprtia-session-transition"
install -Dm644 packaging/stratos/brain-shell-hyprlock.conf \
  "$test_root/prefix/share/hyprtia/brain-shell-hyprlock.conf"
mkdir -p "$test_root/stubs"

cat >"$test_root/stubs/hyprlock" <<'EOF'
#!/bin/sh
printf 'hyprlock\n%s\n' "$*" >"$HYPRTIA_TRANSITION_LOG"
EOF

cat >"$test_root/stubs/hyprshutdown" <<'EOF'
#!/bin/sh
printf 'hyprshutdown\n%s\n' "$*" >"$HYPRTIA_TRANSITION_LOG"
EOF

chmod 755 "$test_root/stubs/hyprlock" "$test_root/stubs/hyprshutdown"
export PATH="$test_root/stubs:$PATH"
export HYPRTIA_TRANSITION_LOG="$test_root/transition.log"

"$test_root/prefix/bin/hyprtia-session-transition" lock
printf 'hyprlock\n-c %s\n' \
  "$test_root/prefix/share/hyprtia/brain-shell-hyprlock.conf" |
  cmp - "$HYPRTIA_TRANSITION_LOG"

"$test_root/prefix/bin/hyprtia-session-transition" logout
printf '%s\n' hyprshutdown '--top-label Logging out...' |
  cmp - "$HYPRTIA_TRANSITION_LOG"

"$test_root/prefix/bin/hyprtia-session-transition" reboot
printf '%s\n' hyprshutdown '--top-label Restarting... --post-cmd systemctl reboot' |
  cmp - "$HYPRTIA_TRANSITION_LOG"

"$test_root/prefix/bin/hyprtia-session-transition" shutdown
printf '%s\n' hyprshutdown '--top-label Shutting down... --post-cmd systemctl poweroff' |
  cmp - "$HYPRTIA_TRANSITION_LOG"

if "$test_root/prefix/bin/hyprtia-session-transition" invalid 2>"$test_root/error.log"; then
  echo "invalid action unexpectedly succeeded" >&2
  exit 1
else
  status=$?
fi
test "$status" -eq 64
grep -q 'usage: hyprtia-session-transition' "$test_root/error.log"
