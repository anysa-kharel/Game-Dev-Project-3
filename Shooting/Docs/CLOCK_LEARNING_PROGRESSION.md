# Clock learning progression — 1.6.1

## Teaching and later application

The course uses the existing C crouch and Space jump controls. Tutorial props sit
on the safe approach after the sector's enemies have been cleared and before its
near-side checkpoint. Later crossings apply these actions on a moving clock hand.

| Stage | Safe introduction | Later application |
| --- | --- | --- |
| Clock 1 | Existing boarding guidance and near-side flag. | Ride from 9 to 3 and step onto the broad dock. |
| Clock 2 | Crouch under an arch on solid ground. | Crouch beneath a fixed low gantry as the hand passes 12. |
| Clock 3 | Hold W and jump from a gold stripe over a low block; solid floor surrounds it. | Move to the hand's gold tip at 3 and jump across the exit gap. |
| Clock 4 | Recap signs and the skills learned on clocks 2/3. | Crouch while riding, stand after clearing the beam, then jump to the far dock. |

The hand stays parked at 3 on the two jump stages, avoiding a repeated wait or a
rushed jump. Near-side checkpoint retries reset the active hand to 9. The course
keeps the eight existing checkpoint indices and sixty-enemy campaign requirements.

## Geometry and feedback

- The practice block is 65 Unreal units high and 100 units long along the route.
  It exceeds the normal 45-unit step height and is below the default jump apex.
- Jump landings on clocks 3/4 start 240 units beyond the 1,200-unit hand tip.
  They remain broad, level, and connected to the existing checkpoint/corridor.
  The actual template serializes Walk_Speed=600 and Crouch_Speed=400. A simple
  420 jump-speed / 980 gravity trajectory has sufficient horizontal range with
  W held before Space. This calculation is not an engine air-control test.
- All seven pieces of each saved jump dock are trimmed: floor, floor stripes,
  side rails, and rail caps. This avoids an invisible floor or rail bridge and
  prevents the old rail ends from obstructing the approach. The floor arrows
  move onto the new landing instead of pointing into the gap.
- Riding gantries are independent, unowned world actors while the hand rotates underneath.
  Their Pawn collision stays active when the character is based on the hand.
  A swept capsule/arc guard pauses the hand before an obstructed rider reaches
  the beam and resumes after crouching gives sufficient clearance.
  Their clearance adapts to the actual capsule heights observed during the
  template's custom bCrouching animation. The calibration does not depend on
  native Character crouch state, which this template does not use for its
  custom crouch timeline. Hand rails remain below the gantry.
- The stand prompt waits until the rider's whole capsule clears the beam.
  Gold tip/landing markers and dark-backed ivory/gold signs show where to go.
  Contextual HUD instructions and the F1 guide explain the same sequence.

## Saved-map upgrade

RefreshClockLessons changes only the matching far-dock scenery for clocks 3/4
and tags it. Reopening or retrying the upgrade writes the same target positions
and lengths; it does not keep shifting the docks. Lesson components are owned by
their clock and replaced by tag on reconstruction, preventing visible duplicates.
The default regeneration path also uses this course. No campaign regeneration
is necessary for the supplied saved map.

The editor script calls ValidateClockLessons after refreshing or generating a
map. It checks all four lesson stages, generated lesson geometry, and all seven
trimmed pieces on each jump dock before saving and reporting READY. It reports
failure if those expected saved-map structures cannot be found. In the uploaded
map, all fourteen expected dock-piece positions and the original size vectors
were found in the serialized data before modification.

## Verification

- 6,778 engine-independent C++ checks pass, including 170 lesson checks and
  1,019 gantry collision regressions. The new checks cover standing/crouched
  riders, partial animations, early uncrouch, multiple radii/frame rates, rail
  riders, curved return paths, and large frame steps.
  These cover stage selection, parked exits at multiple frame rates, pause and
  retry, the modeled jump path, practice-block clearance, adaptive ceiling
  heights, safe checkpoint positions, dock selection/idempotence, and HUD cues.
- 23 Python tests pass using editor stand-ins: 15 saved-map/startup checks and
  eight audio-import checks. A failed lesson validation prevents a successful
  save/preparation result. The pre-existing audio test expectation now includes
  the already-present reload sound and verifies that it cannot loop.
- Original input configuration, enemy code/balance, reload/music implementation,
  ammo bindings, and all uploaded .uasset/.umap files were preserved byte for byte.
- Python syntax, Unreal generated-header order, changed-file scope, and ZIP
  integrity are checked for delivery.

No Unreal Editor or target SDK is installed in this environment. Native module
compilation, actual map preparation, capsule/platform collisions, safe dismounts,
rendered text, perceived difficulty, completion time, and packaged releases need
local testing. These independent checks do not establish a grading outcome.

## Local playtest

1. Rebuild and wait for TICK TOCK READY / GANTRY COLLISION FIX.
2. Confirm Clock 1 is the original basic ride with no crouch or jump obstruction.
3. After clearing sector 2, test the safe arch standing/crouching. Ride Clock 2
   under the gantry and check that standing is blocked while crouching fits.
4. After clearing sector 3, jump the practice block from the stripe, with W held
   before Space. Confirm Clock 3 parks at 3 and its landing/rails leave a visible
   gap. Jump from the gold tip at normal walking speed and reach flag 6.
5. On Clock 4, crouch through the beam, follow the stand prompt, then jump to
   flag 8. Check both crouch input styles supported by the template if used.
6. Miss a jump and retry with Backspace. Confirm the near checkpoint, health,
   ammo, defeated robots, and reset hand behave as before. Test pause and replay.
7. Reopen the editor and check that lesson props and dock offsets do not stack.
   Run the complete campaign, record its new duration and player feedback, then
   rebuild and test platform releases.

Primary APIs checked:
- https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UCapsuleComponent/GetScaledCapsuleHalfHeight
- https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UCharacterMovementComponent/GetMaxJumpHeight
- https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/AActor/GetComponents
- https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/ACharacter

See GANTRY_COLLISION_FIX.md for the independent-obstacle migration and local test.
