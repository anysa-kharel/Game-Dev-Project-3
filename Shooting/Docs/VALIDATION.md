# Validation — Clock Arena 1.4.0

Current update: controls briefing and text contrast, November 6, 2026. See
CONTROLS_AND_READABILITY.md for changes and local Unreal checks. Earlier
lighting/campaign notes are historical records of those updates.

| Check | Result | Limit |
| --- | --- | --- |
| Existing C++ combat and layout rules | 1,673 passed | Independent C++20, not Unreal classes |
| Existing wayfinding and flags | 794 passed | Logical route/boarding checks |
| Existing campaign and encounters | 1,655 passed | No engine physics or human timing |
| Controls start flow, contrast, viewport bounds | 1,366 passed | Shared rules/layout; no real device or engine rendering |
| Editor startup and saved-map upgrade | 13 passed | Python stand-ins for editor subsystems |
| Source input bindings | Checked | Original Enhanced Input mapping and native key bindings |
| Original .uasset/.umap preservation | 217 unchanged | Compared to Shootingv3.zip before local editor upgrade |
| DefaultInput.ini preservation | Unchanged | Compared to the uploaded ZIP |
| Controls layout | Visually inspected as a code-rendered reference | Not an Unreal screenshot |
| Python and project metadata syntax | Checked | No editor execution |
| Delivery ZIP | CRC and contents checked | Build caches excluded |

Run bash Tests/run_rules_tests.sh to reproduce the independent tests.
Total: 5,488 C++ checks and 13 Python tests. A passing rules suite does not
compile Unreal classes, run Unreal Header Tool, validate materials in the engine,
or exercise mouse capture and Blueprint integration.

The user reports that the uploaded version works perfectly. This update's native
Unreal build, controls screen, and sign rendering still require a local build
and playtest. No Unreal Editor or target platform SDK is installed here.

Primary API references checked for this update:

- [Epic: HUD hitboxes](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/AHUD)
- [Epic: Canvas text outlines and scale](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/FCanvasTextItemBase)
- [Epic: Text render component](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UTextRenderComponent)
- [Epic: Widget Blueprint library](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/UMG/UWidgetBlueprintLibrary)
