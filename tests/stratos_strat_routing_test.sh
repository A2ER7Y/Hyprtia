#!/bin/sh

set -eu

strat_runner=$1
test_root=$(mktemp -d)
trap 'rm -rf "$test_root"' EXIT HUP INT TERM

install -Dm755 "$strat_runner" "$test_root/bin/hyprtia-strat"

cat >"$test_root/bin/strat" <<'EOF'
#!/bin/sh
if [ "${1:-}" = -r ]; then
  shift
fi
target=$1
shift
if [ "${1:-}" = sh ] && [ "${2:-}" = -c ]; then
  command_name=${5:-}
  case "$target:$command_name" in
    kirolinux:hyprctl | kirolinux:fallback-app | archbase:hyprctl | fedora:firefox | appbase:firefox) exit 0 ;;
    *) exit 1 ;;
  esac
fi
printf '%s|%s\n' "$target" "$*" >>"$HYPRTIA_STRAT_TEST_LOG"
EOF
chmod 755 "$test_root/bin/strat"

export PATH="$test_root/bin:/usr/bin:/bin"
export HYPRTIA_FORCE_BEDROCK=1
export HYPRTIA_STRAT_TEST_LOG="$test_root/strat.log"

"$test_root/bin/hyprtia-strat" --check system hyprctl
"$test_root/bin/hyprtia-strat" system hyprctl reload
"$test_root/bin/hyprtia-strat" apps firefox --new-window
"$test_root/bin/hyprtia-strat" apps fallback-app
if "$test_root/bin/hyprtia-strat" apps missing-command; then
  echo "missing cross-stratum command unexpectedly succeeded" >&2
  exit 1
else
  test "$?" -eq 127
fi

printf '%s\n' \
  'kirolinux|hyprctl reload' \
  'fedora|firefox --new-window' \
  'kirolinux|fallback-app' | cmp - "$HYPRTIA_STRAT_TEST_LOG"

: >"$HYPRTIA_STRAT_TEST_LOG"
export HYPRTIA_SYSTEM_STRATUM=archbase
export HYPRTIA_APPS_STRATUM=appbase
"$test_root/bin/hyprtia-strat" system hyprctl reload
"$test_root/bin/hyprtia-strat" apps firefox
printf '%s\n' \
  'archbase|hyprctl reload' \
  'appbase|firefox' | cmp - "$HYPRTIA_STRAT_TEST_LOG"

unset HYPRTIA_FORCE_BEDROCK HYPRTIA_SYSTEM_STRATUM HYPRTIA_APPS_STRATUM
cat >"$test_root/bin/local-app" <<'EOF'
#!/bin/sh
printf '%s\n' "$*" >"$HYPRTIA_LOCAL_TEST_LOG"
EOF
chmod 755 "$test_root/bin/local-app"
export HYPRTIA_LOCAL_TEST_LOG="$test_root/local.log"
"$test_root/bin/hyprtia-strat" apps local-app hello
printf '%s\n' hello | cmp - "$HYPRTIA_LOCAL_TEST_LOG"
