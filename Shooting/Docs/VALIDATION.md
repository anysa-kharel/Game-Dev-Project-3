# Validation and first local playtest

Clock Arena expansion, 2026-11-04, for Unreal Engine 5.8.

## Checks actually performed

| Check | Result | Practical limit |
| --- | --- | --- |
| C++20 compilation and execution of `CombatRules.h` and `ArenaLayout.h` tests | 1,673 checks passed with `-Wall -Wextra -pedantic` | Does not compile Unreal classes |
| Clock phase and boarding logic | Passed | Includes continuity across a complete sampled cycle, near/far dock pauses, and configurable period scaling |
| Health state | Passed | Damage, death, delayed recovery, reset, and healing only for time beyond the six-second threshold |
| Attack state | Passed | Visible charging, cooldown, cancellation after lost sight or range, fresh charge on reacquisition, and melee warning |
| Layout data | Passed | Six enemies per area; all spawn capsules clear the 19 configured cover boxes; clock dock overlap, gap size, and ramp slope estimates |
| Uploaded Content preservation | Passed | SHA-256 compared all 205 original Content files against the upload; every one remained identical |
| Original input configuration | Passed | Byte-for-byte comparison with uploaded `DefaultInput.ini` |
| Python / JSON / shell syntax | Passed | Editor scripts are parsed, not executed inside Unreal |
| Added sound assets | Passed | Seven nonempty mono 16-bit 22050 Hz WAVs; all native sound references resolve to included source WAVs |
| Native module and source consistency | Checked | Module identifiers, generated-header placement, includes, V7 targets, and prior known UE 5.8 incompatibilities |
| Delivery ZIP | Verified during packaging | CRC integrity, required project paths, and exclusion of build caches |

Reproduce the standard C++ checks on a machine with a C++ compiler:

```bash
bash Tests/run_rules_tests.sh
```

The tests use actual gameplay rule functions and the actual spawn/cover layout data. They do not simulate Unreal physics, line traces, dynamic bases, or human play.

The uploaded Bullet Blueprint contains `ApplyDamage` and a physics-simulation check before applying an impulse. The new enemy's native `TakeDamage` receives those gameplay damage calls. The expansion health manager subscribes to the original player's `OnTakeAnyDamage` event. The original player and controller classes are loaded directly by the new game mode. This integration still requires verification in Unreal.

## First build and playtest required

1. Build with the included platform launch helper. Confirm Unreal Header Tool and C++ compilation succeed. The source uses UE 5.8 V7 build settings; keep the first real error message if a build fails.
2. Confirm the first editor script creates `/Game/ClockArena/Maps/L_ClockArena`, the shared material, and seven sound assets. Open the generated map and save it.
3. Start Play. Confirm the original player, rifle, input, animations, ammo UI, aim, and reload still work.
4. Shoot a guard three times and a gold scout twice. Confirm hit feedback, debris, and the hostile counter. Empty a magazine and reload.
5. Watch a guard charge and fire. Confirm a clear projectile damages the player, a crate stops it, and moving out of sight cancels a pending attack.
6. Let a scout approach. Confirm it warns before striking and cannot damage the player through a wall.
7. Use both gallery ramps. Test hiding under a platform and crouching behind a low crate. Confirm health starts recovering after six seconds without damage.
8. Enter the near checkpoint, board the clock hand, and remain aboard for the full rotation. Test the side rails, dock overlap, and boarding pauses. Verify that crossing into the foundry saves the far checkpoint.
9. Fall off the clock and get defeated by enemies. Confirm correct respawn position, full health, refilled magazine, preserved defeated enemies, and reset surviving enemies.
10. Clear all 12 robots. Confirm the exit stays locked before the counter reaches zero, then opens, and the green beacon displays victory. Test Enter to play again.
11. Test P to pause/resume, F1 for instructions, and Backspace to retry.
12. Package a native build and repeat the complete route outside the editor. Observe frame rate and input capture on the target Mac or Windows computer.

## Honest limits

No Unreal Editor, engine headers, Apple SDK, or Windows Unreal toolchain are installed in the development environment. The complete Unreal project has therefore not been compiled, opened, or playtested here. Native API compatibility, Blueprint integration, material creation, map serialization, collision, moving-base carriage, and packaged builds remain unverified until the local steps above are completed.

No human playtest observations, measured completion time, or performance results have been invented. Record actual observations below and revise the level from that evidence.

| Test / device | Observed problem | Change | Result of another test |
| --- | --- | --- | --- |
| To be recorded | — | — | — |

Primary API references consulted:

- Epic, `AActor::TakeDamage`: https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/AActor/TakeDamage
- Epic, UE 5.8 `ACharacter::SetBase` overloads: https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/ACharacter/SetBase
- Epic, `UWorld` actor-spawn delegates: https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UWorld


## November 5 lighting and checkpoint update

See LIGHTING_AND_CHECKPOINTS.md for the current 1.2.0 changes and checks. This
revision is based on the latest uploaded Shooting.zip. All 217 original non-Python
Content files and DefaultInput.ini were compared with that ZIP and preserved
byte for byte. The saved map receives its visual upgrade inside Unreal.
