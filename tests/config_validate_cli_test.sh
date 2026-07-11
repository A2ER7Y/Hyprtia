#!/bin/sh

set -eu

noctalia_bin=$1

fail() {
  printf '%s\n' "config_validate_cli_test: FAIL: $*" >&2
  exit 1
}

valid_output=$("$noctalia_bin" config validate tests/config_validate/generated-config 2>&1) \
  || fail "generated single-file config should validate"
case "$valid_output" in
  *"Config is valid"*) ;;
  *) fail "generated single-file config did not print success" ;;
esac

warn_output=$("$noctalia_bin" config validate tests/config_validate/warn-only.toml 2>&1) \
  || fail "warning-only config should validate"
case "$warn_output" in
  *"WARN  shell.ui_scl: unknown setting"*) ;;
  *) fail "warning-only config did not report the unknown setting" ;;
esac

widget_output=$("$noctalia_bin" config validate tests/config_validate/widget-schema.toml 2>&1) \
  || fail "built-in widget schema should validate without Settings UI discovery"
case "$widget_output" in
  *"Config is valid"*) ;;
  *) fail "built-in widget schema did not print success" ;;
esac

syntax_output=$("$noctalia_bin" config validate tests/config_validate/syntax-error.toml 2>&1) \
  && fail "syntax-error config should fail"
case "$syntax_output" in
  *"ERROR syntax: tests/config_validate/syntax-error.toml:"*) ;;
  *) fail "syntax-error config did not report the source path" ;;
esac
