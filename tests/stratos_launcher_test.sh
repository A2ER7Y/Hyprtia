#!/bin/sh

set -eu

launcher=$1
test_root=$(mktemp -d)
trap 'rm -rf "$test_root"' EXIT HUP INT TERM

install -Dm755 "$launcher" "$test_root/prefix/bin/hyprtia"
install -Dm644 packaging/stratos/config.toml "$test_root/prefix/share/hyprtia/config.toml"

cat >"$test_root/prefix/bin/noctalia" <<'EOF'
#!/bin/sh
printf '%s\n' "$@" >"$HYPRTIA_TEST_ARGS"
exit "${HYPRTIA_TEST_EXIT:-0}"
EOF
chmod 755 "$test_root/prefix/bin/noctalia"

export HOME="$test_root/home"
export XDG_CONFIG_HOME="$test_root/config"
export HYPRTIA_TEST_ARGS="$test_root/args"

"$test_root/prefix/bin/hyprtia" msg status
cmp packaging/stratos/config.toml "$XDG_CONFIG_HOME/noctalia/stratos.toml"
printf '%s\n' msg status | cmp - "$HYPRTIA_TEST_ARGS"

rm "$XDG_CONFIG_HOME/noctalia/stratos.toml"
printf '%s\n' '[shell]' 'telemetry_enabled = false' >"$XDG_CONFIG_HOME/noctalia/custom.toml"
"$test_root/prefix/bin/hyprtia" --version
test ! -e "$XDG_CONFIG_HOME/noctalia/stratos.toml"
printf '%s\n' --version | cmp - "$HYPRTIA_TEST_ARGS"

export HYPRTIA_TEST_EXIT=17
if "$test_root/prefix/bin/hyprtia"; then
  echo "launcher did not preserve the Noctalia exit status" >&2
  exit 1
else
  status=$?
fi
test "$status" -eq 17
