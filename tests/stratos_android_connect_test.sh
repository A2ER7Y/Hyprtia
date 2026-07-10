#!/bin/sh

set -eu

launcher=$1
test_root=$(mktemp -d)
trap '/usr/bin/rm -rf "$test_root"' EXIT HUP INT TERM

install -Dm755 "$launcher" "$test_root/hyprtia-android-connect"
mkdir -p "$test_root/config/noctalia/plugins/androidconnect" "$test_root/stubs"
printf '{}\n' >"$test_root/config/noctalia/plugins/androidconnect/manifest.json"

cat >"$test_root/stubs/qs" <<'EOF'
#!/bin/sh
printf 'qs\n%s\n' "$*" >"$HYPRTIA_ANDROID_TEST_LOG"
EOF
chmod 755 "$test_root/stubs/qs"

export HOME="$test_root/home"
export XDG_CONFIG_HOME="$test_root/config"
export HYPRTIA_ANDROID_TEST_LOG="$test_root/result"
export PATH="$test_root/stubs"

"$test_root/hyprtia-android-connect"
printf '%s\n' qs 'ipc call plugin:androidconnect toggle' | /usr/bin/cmp - "$HYPRTIA_ANDROID_TEST_LOG"

/usr/bin/rm "$test_root/stubs/qs"
/usr/bin/cat >"$test_root/stubs/kdeconnect-app" <<'EOF'
#!/bin/sh
printf 'kdeconnect-app\n' >"$HYPRTIA_ANDROID_TEST_LOG"
EOF
/usr/bin/chmod 755 "$test_root/stubs/kdeconnect-app"
"$test_root/hyprtia-android-connect"
printf 'kdeconnect-app\n' | /usr/bin/cmp - "$HYPRTIA_ANDROID_TEST_LOG"

/usr/bin/rm "$test_root/stubs/kdeconnect-app"
/usr/bin/cat >"$test_root/stubs/kdeconnect-cli" <<'EOF'
#!/bin/sh
printf 'kdeconnect-cli\n%s\n' "$*" >"$HYPRTIA_ANDROID_TEST_LOG"
EOF
/usr/bin/chmod 755 "$test_root/stubs/kdeconnect-cli"
"$test_root/hyprtia-android-connect"
printf '%s\n' kdeconnect-cli --list-devices | /usr/bin/cmp - "$HYPRTIA_ANDROID_TEST_LOG"
