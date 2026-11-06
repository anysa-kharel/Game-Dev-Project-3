#!/bin/bash
set -euo pipefail
CA_TEST_DIR="$(cd "$(dirname "$0")" && pwd)"
CA_TEST_BIN="$(mktemp -d)"
trap 'rm -rf "$CA_TEST_BIN"' EXIT
c++ -std=c++20 -Wall -Wextra -pedantic "$CA_TEST_DIR/combat_rules_test.cpp" -o "$CA_TEST_BIN/combat_rules"
"$CA_TEST_BIN/combat_rules"
c++ -std=c++20 -Wall -Wextra -pedantic "$CA_TEST_DIR/wayfinding_test.cpp" -o "$CA_TEST_BIN/wayfinding"
"$CA_TEST_BIN/wayfinding"
c++ -std=c++20 -Wall -Wextra -pedantic "$CA_TEST_DIR/campaign_test.cpp" -o "$CA_TEST_BIN/campaign"
"$CA_TEST_BIN/campaign"
c++ -std=c++20 -Wall -Wextra -pedantic "$CA_TEST_DIR/interface_test.cpp" -o "$CA_TEST_BIN/interface"
"$CA_TEST_BIN/interface"
c++ -std=c++20 -Wall -Wextra -pedantic "$CA_TEST_DIR/presentation_audio_test.cpp" -o "$CA_TEST_BIN/presentation_audio"
"$CA_TEST_BIN/presentation_audio"
c++ -std=c++20 -Wall -Wextra -pedantic "$CA_TEST_DIR/clock_lessons_test.cpp" -o "$CA_TEST_BIN/clock_lessons"
"$CA_TEST_BIN/clock_lessons"
python3 "$CA_TEST_DIR/editor_upgrade_test.py"
python3 "$CA_TEST_DIR/audio_import_test.py"
