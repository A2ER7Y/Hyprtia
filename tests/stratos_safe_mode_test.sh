#!/bin/sh

set -eu

launcher=$1
normal_config=$2
safe_config=$3
test_root=$(mktemp -d)
trap 'rm -rf "$test_root"' EXIT HUP INT TERM

install -Dm755 "$launcher" "$test_root/prefix/bin/hyprtia"
install -Dm644 "$normal_config" "$test_root/prefix/share/hyprtia/config.toml"
install -Dm644 "$safe_config" "$test_root/prefix/share/hyprtia/safe-mode.toml"

cat >"$test_root/prefix/bin/noctalia" <<'EOF'
#!/bin/sh
{
  printf 'safe=%s\n' "${HYPRTIA_SAFE_MODE:-0}"
  printf 'config=%s\n' "${NOCTALIA_CONFIG_HOME:-}"
  printf 'state=%s\n' "${NOCTALIA_STATE_HOME:-}"
  printf 'data=%s\n' "${NOCTALIA_DATA_HOME:-}"
  printf 'guard=%s\n' "${HYPRTIA_STARTUP_GUARD_FILE:-}"
} >"$HYPRTIA_SAFE_TEST_ENV"
if [ "${HYPRTIA_TEST_READY:-0}" = 1 ] && [ -n "${HYPRTIA_STARTUP_GUARD_FILE:-}" ]; then
  rm -f "$HYPRTIA_STARTUP_GUARD_FILE"
fi
exit "${HYPRTIA_TEST_EXIT:-0}"
EOF
chmod 755 "$test_root/prefix/bin/noctalia"

export HOME="$test_root/home"
export XDG_CONFIG_HOME="$test_root/config"
export XDG_STATE_HOME="$test_root/state"
export XDG_CACHE_HOME="$test_root/cache"
export HYPRTIA_SAFE_TEST_ENV="$test_root/environment"
export HYPRTIA_SAFE_MODE_NOTIFY=0

mkdir -p "$XDG_CACHE_HOME/noctalia"
printf '%s\n' 'startup failed in widget module' >"$XDG_CACHE_HOME/noctalia/noctalia.log"

mkdir -p "$XDG_STATE_HOME/hyprtia"
sleep 10 &
live_pid=$!
printf '%s\n' "$live_pid" >"$XDG_STATE_HOME/hyprtia/startup.pending"
"$test_root/prefix/bin/hyprtia"
grep -qx 'safe=0' "$HYPRTIA_SAFE_TEST_ENV"
grep -qx 'guard=' "$HYPRTIA_SAFE_TEST_ENV"
test ! -e "$XDG_STATE_HOME/hyprtia/safe-mode.active"
kill "$live_pid"
wait "$live_pid" 2>/dev/null || true
rm "$XDG_STATE_HOME/hyprtia/startup.pending"

export HYPRTIA_TEST_EXIT=17
if "$test_root/prefix/bin/hyprtia"; then
  echo "crashing startup unexpectedly succeeded" >&2
  exit 1
else
  test "$?" -eq 17
fi
test -f "$XDG_STATE_HOME/hyprtia/startup.pending"
test ! -e "$XDG_STATE_HOME/hyprtia/safe-mode.active"

export HYPRTIA_TEST_EXIT=0
export HYPRTIA_TEST_READY=1
"$test_root/prefix/bin/hyprtia"

safe_root=$XDG_STATE_HOME/hyprtia/safe-mode
grep -qx 'safe=1' "$HYPRTIA_SAFE_TEST_ENV"
grep -qx "config=$safe_root/config" "$HYPRTIA_SAFE_TEST_ENV"
grep -qx "state=$safe_root/state" "$HYPRTIA_SAFE_TEST_ENV"
grep -qx "data=$safe_root/data" "$HYPRTIA_SAFE_TEST_ENV"
test -f "$XDG_STATE_HOME/hyprtia/safe-mode.active"
test ! -e "$XDG_STATE_HOME/hyprtia/startup.pending"
cmp "$safe_config" "$safe_root/config/noctalia/safe-mode.toml"
cmp "$XDG_CACHE_HOME/noctalia/noctalia.log" "$XDG_STATE_HOME/hyprtia/last-startup.log"

printf '\n# local frozen marker\n' >>"$safe_root/config/noctalia/safe-mode.toml"
"$test_root/prefix/bin/hyprtia"
grep -q 'local frozen marker' "$safe_root/config/noctalia/safe-mode.toml"

"$test_root/prefix/bin/hyprtia" --safe-mode-reset
test ! -e "$XDG_STATE_HOME/hyprtia/safe-mode.active"
test ! -e "$XDG_STATE_HOME/hyprtia/startup.pending"

unset HYPRTIA_TEST_READY
export HYPRTIA_TEST_EXIT=1
if "$test_root/prefix/bin/hyprtia" msg status; then
  echo "failing CLI command unexpectedly succeeded" >&2
  exit 1
fi
test ! -e "$XDG_STATE_HOME/hyprtia/startup.pending"
