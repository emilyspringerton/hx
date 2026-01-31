#!/usr/bin/env bash
set -euo pipefail

TEST_TOTAL=0
TEST_FAILED=0

start_test() {
  local name="$1"
  TEST_TOTAL=$((TEST_TOTAL + 1))
  printf "[TEST] %s\n" "$name"
}

pass_test() {
  printf "[PASS] %s\n" "$1"
}

fail_test() {
  local name="$1"
  local msg="$2"
  TEST_FAILED=$((TEST_FAILED + 1))
  printf "[FAIL] %s: %s\n" "$name" "$msg" >&2
}

assert_equal() {
  local expected="$1"
  local actual="$2"
  local name="$3"
  if [[ "$expected" != "$actual" ]]; then
    fail_test "$name" "expected '$expected' got '$actual'"
    return 1
  fi
}

assert_contains() {
  local haystack="$1"
  local needle="$2"
  local name="$3"
  if [[ "$haystack" != *"$needle"* ]]; then
    fail_test "$name" "expected to find '$needle'"
    return 1
  fi
}

finish_tests() {
  printf "[SUMMARY] %d tests, %d failures\n" "$TEST_TOTAL" "$TEST_FAILED"
  if [[ "$TEST_FAILED" -ne 0 ]]; then
    exit 1
  fi
}
