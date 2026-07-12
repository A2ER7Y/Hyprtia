#!/bin/sh

set -eu

debug_report=$1
strat_runner=$2
test_root=$(mktemp -d)
trap 'rm -rf "$test_root"' EXIT HUP INT TERM

install -Dm755 "$debug_report" "$test_root/prefix/bin/hyprtia-debug-report"
install -Dm755 "$strat_runner" "$test_root/prefix/bin/hyprtia-strat"
mkdir -p "$test_root/stubs"

cat >"$test_root/prefix/bin/noctalia" <<'EOF'
#!/bin/sh
printf '%s\n' "$*" >"$HYPRTIA_NOTIFICATION_LOG"
EOF
chmod 755 "$test_root/prefix/bin/noctalia"

cat >"$test_root/stubs/brl" <<'EOF'
#!/bin/sh
printf '%s\n' 'kirolinux: enabled' 'fedora: enabled'
EOF
cat >"$test_root/stubs/Hyprland" <<'EOF'
#!/bin/sh
printf '%s\n' 'Hyprland 0.55 test'
EOF
cat >"$test_root/stubs/journalctl" <<'EOF'
#!/bin/sh
printf '%s\n' 'service started' 'token=journal-secret'
EOF
cat >"$test_root/stubs/wl-copy" <<'EOF'
#!/bin/sh
cp /dev/stdin "$HYPRTIA_CLIPBOARD_LOG"
EOF
chmod 755 "$test_root/stubs/brl" "$test_root/stubs/Hyprland" "$test_root/stubs/journalctl" "$test_root/stubs/wl-copy"

export HOME="$test_root/home"
export XDG_STATE_HOME="$test_root/state"
export XDG_CACHE_HOME="$test_root/cache"
export HYPRTIA_NOTIFICATION_LOG="$test_root/notification.log"
export HYPRTIA_CLIPBOARD_LOG="$test_root/clipboard.log"
export PATH="$test_root/stubs:/usr/bin:/bin"
mkdir -p "$XDG_STATE_HOME/hyprtia" "$XDG_CACHE_HOME/noctalia"
printf '%s\n' "failure under $HOME" 'password=unsafe' >"$XDG_STATE_HOME/hyprtia/last-startup.log"
printf '%s\n' 'shell log line' 'authorization: bearer unsafe' >"$XDG_CACHE_HOME/noctalia/noctalia.log"

report=$("$test_root/prefix/bin/hyprtia-debug-report")
test "$report" = "$XDG_STATE_HOME/hyprtia/debug-report.txt"
test -f "$report"
cmp "$report" "$HYPRTIA_CLIPBOARD_LOG"
grep -q 'Hyprland 0.55 test' "$report"
grep -q 'kirolinux: enabled' "$report"
grep -q '\[REDACTED SECRET LINE\]' "$report"
! grep -q 'journal-secret\|unsafe' "$report"
! grep -q "$HOME" "$report"
grep -q 'notification-show Rapport de debug Hyprtia' "$HYPRTIA_NOTIFICATION_LOG"
