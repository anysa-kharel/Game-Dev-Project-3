#!/bin/bash
set -euo pipefail

CA_PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
CA_PROJECT="$CA_PROJECT_DIR/Shooting.uproject"
CA_UE_ROOT=""
if [ "$#" -gt 0 ]; then
    CA_UE_ROOT="$1"
fi
if [ -z "$CA_UE_ROOT" ]; then
    for CA_CANDIDATE in "/Users/Shared/Epic Games/UE_5.8" "/Users/Shared/Epic Games/UE_5.8_SI"; do
        if [ -f "$CA_CANDIDATE/Engine/Build/BatchFiles/Mac/Build.sh" ]; then
            CA_UE_ROOT="$CA_CANDIDATE"
            break
        fi
    done
fi
if [ ! -f "$CA_UE_ROOT/Engine/Build/BatchFiles/Mac/Build.sh" ]; then
    echo "Unreal Engine 5.8 was not found at its usual location."
    echo "Run this helper with your engine folder as its argument:"
    echo "  bash Launch_Mac.command '/path/to/UE_5.8'"
    exit 1
fi
if ! /usr/bin/xcrun --find clang >/dev/null 2>&1; then
    echo "Xcode is not selected. Open Xcode > Settings > Locations and select its Command Line Tools."
    exit 1
fi
CA_EDITOR="$CA_UE_ROOT/Engine/Binaries/Mac/UnrealEditor.app"
if [ ! -d "$CA_EDITOR" ]; then
    echo "UnrealEditor.app was not found in the selected engine installation."
    exit 1
fi
mkdir -p "$CA_PROJECT_DIR/Saved/BuildLogs"
CA_LOG="$CA_PROJECT_DIR/Saved/BuildLogs/first_mac_build.log"
echo "Building Clock Arena for your Mac. The first build can take several minutes."
echo "This builds the small game module, not the Unreal Engine source."
echo "Build log: $CA_LOG"
/bin/bash "$CA_UE_ROOT/Engine/Build/BatchFiles/Mac/Build.sh" \
    ShootingEditor Mac Development "$CA_PROJECT" \
    -waitmutex -MaxParallelActions=1 -NoHotReloadFromIDE 2>&1 | tee "$CA_LOG"
echo "Opening Unreal. The first startup creates the included map, material and sounds."
echo "When ready: press Play and click the viewport."
/usr/bin/open -n -a "$CA_EDITOR" --args "$CA_PROJECT"
