#!/bin/sh

set -eu

launcher=$1
test_root=$(mktemp -d)
trap '/usr/bin/rm -rf "$test_root"' EXIT HUP INT TERM

install -Dm755 "$launcher" "$test_root/hyprtia-assistant"
mkdir -p "$test_root/stubs"

/usr/bin/cat >"$test_root/stubs/nyarchassistant" <<'EOF'
#!/bin/sh
printf 'nyarchassistant\n%s\n' "$*" >"$HYPRTIA_ASSISTANT_TEST_LOG"
EOF
/usr/bin/chmod 755 "$test_root/stubs/nyarchassistant"

export HYPRTIA_ASSISTANT_TEST_LOG="$test_root/result"
export PATH="$test_root/stubs"

"$test_root/hyprtia-assistant" --new-chat "bonjour"
printf '%s\n' nyarchassistant '--new-chat bonjour' | /usr/bin/cmp - "$HYPRTIA_ASSISTANT_TEST_LOG"

/usr/bin/rm "$test_root/stubs/nyarchassistant"
/usr/bin/cat >"$test_root/stubs/notify-send" <<'EOF'
#!/bin/sh
printf 'notify-send\n%s\n' "$*" >"$HYPRTIA_ASSISTANT_TEST_LOG"
EOF
/usr/bin/chmod 755 "$test_root/stubs/notify-send"

if "$test_root/hyprtia-assistant" 2>"$test_root/stderr"; then
  printf '%s\n' "launcher unexpectedly succeeded without nyarchassistant" >&2
  exit 1
fi

/usr/bin/cmp "$test_root/stderr" - <<'EOF'
hyprtia-assistant: nyarchassistant is not installed; install hyprtia-assistant
EOF
/usr/bin/cmp "$HYPRTIA_ASSISTANT_TEST_LOG" - <<'EOF'
notify-send
Hyprtia Assistant Installez le paquet hyprtia-assistant pour activer l’assistant compact.
EOF
