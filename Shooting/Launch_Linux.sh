#!/bin/bash
set -euo pipefail
CA_PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
if [ "$#" -lt 1 ]; then
    echo "Usage: bash Launch_Linux.sh /path/to/UnrealEngine_5.8"
    exit 1
fi
CA_UE_ROOT="$1"
CA_PROJECT="$CA_PROJECT_DIR/Shooting.uproject"
if [ ! -f "$CA_UE_ROOT/Engine/Build/BatchFiles/Linux/Build.sh" ]; then
    echo "The selected engine folder does not contain the Linux build tools."
    exit 1
fi
bash "$CA_UE_ROOT/Engine/Build/BatchFiles/Linux/Build.sh" \
    ShootingEditor Linux Development "$CA_PROJECT" \
    -waitmutex -MaxParallelActions=2
exec "$CA_UE_ROOT/Engine/Binaries/Linux/UnrealEditor" "$CA_PROJECT"
