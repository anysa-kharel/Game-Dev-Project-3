#!/bin/bash
set -euo pipefail
CA_TEST_DIR="$(cd "$(dirname "$0")" && pwd)"
CA_TEST_BIN="$(mktemp -d)"
trap 'rm -rf "$CA_TEST_BIN"' EXIT
c++ -std=c++20 -Wall -Wextra -pedantic "$CA_TEST_DIR/combat_rules_test.cpp" -o "$CA_TEST_BIN/combat_rules"
"$CA_TEST_BIN/combat_rules"
c++ -std=c++20 -Wall -Wextra -pedantic "$CA_TEST_DIR/wayfinding_test.cpp" -o "$CA_TEST_BIN/wayfinding"
"$CA_TEST_BIN/wayfinding"
python3 "$CA_TEST_DIR/editor_upgrade_test.py"
