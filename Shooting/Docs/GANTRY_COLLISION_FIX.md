# Blocking gantries on rotating clocks — 1.6.1

## Problem and correction

The riding beam previously belonged to the same ACAClockBridge actor as the
rideable minute hand. Based-movement collision can ignore the actor providing
its movement base. This is consistent with the reported behavior: the practice
arch blocks a player based on the floor, but the clock's own beam lets a player
based on its hand pass. The code structure was verified; the engine behavior
has not been reproduced in this environment.

The fix uses two layers:

1. PrepareRidingGantry creates/reuses one ACAScenery actor for Clock 2 and one
   for Clock 4. Each stays unowned and unattached to the clock, with full query /
   physics collision and an explicit Block response to Pawn. Old clock-owned
   beam components are removed by the existing lesson reconstruction. Tagged
   actors are reused and duplicates/orphans are removed on refresh.
2. Before rotating, ConstrainGantryAdvance sweeps conservative capsule bounds
   through the rider's proposed curved path. A blocked standing rider stops
   the hand just before the beam. It resumes when actual crouch clearance is
   sufficient. A crouch button press alone does not permit a still-tall capsule.
   Early uncrouch guards standing dimensions while preserving the feet. The
   guard covers the hand and its rotating rails, with a step-down hint if needed.

The guard samples at no more than two degrees per arc segment and bounds an
extreme hitch to one revolution, avoiding endpoint-only tunneling and false
collisions on the clear return arc. It uses three units of clearance padding.
It conservatively treats capsule bounds as an expanded box for prediction;
Unreal still provides the actual physical collision against the independent beam.
Character movement's fast attached-movement shortcut is explicitly disabled.

ValidateClockLessons now requires one independent, blocking gantry per crouch
stage before editor preparation reports READY. The saved-map refresh performs
the migration without rebuilding the campaign. The actor and component changes
are applied inside Unreal; uploaded .uasset/.umap bytes remain unchanged in the
supplied ZIP. No external assets or engine modules were added.

## Checks

- 1,019 new engine-independent gantry collision checks pass. Cases include
  standing, actual crouch, partial crouch, early uncrouch, five frame intervals,
  four rider radii, both stages, repeated blocked ticks, rails, initial overlaps,
  return arcs, and large hitches. These are mathematical/logic regressions.
- All 5,759 prior independent C++ checks pass, including the 170 clock lesson checks.
- All 23 Python preparation/import tests pass with editor stand-ins.
- Original maps/assets, enemy behavior, jumping course dimensions, input mapping,
  music/reload implementation, and existing ammo widget code are preserved.
- Python syntax, generated-header placement, source scope, and delivery ZIP
  integrity are checked before delivery.

The environment has no Unreal Editor or target SDK. Full native compilation,
physics, actor migration/serialization, input timing, visible prompts, and the
packaged game require local testing. Passing these logic tests does not establish
that an Unreal playtest has passed.

## Local playtest

1. Close the editor, merge all patch files or use the full project, rebuild, and
   wait for TICK TOCK READY / GANTRY COLLISION FIX. Save the prepared level.
2. On Clock 2, stay standing. Confirm the hand stops before the beam and the
   prompt asks for C. Walk toward the beam and confirm solid collision.
3. Crouch: the hand should wait until the capsule fits, then resume. Stay low
   until clear, stand, and step onto the far dock.
4. Repeat on Clock 4, including its exit jump. Release crouch too early and
   verify the obstruction still stops movement; crouch again to continue.
5. Check standing/crouching on the rotating rails, checkpoint retry, pause,
   repeated Play, and reopening the editor. There should be two tagged
   independent gantry actors and no duplicates. Check the ground practice arch.
6. Rebuild and test platform releases after the editor checks pass.

Primary APIs checked:
- https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/EMoveComponentFlags
- https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UCharacterMovementComponent
- https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FActorSpawnParameters
- https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UWorld/SpawnActor
- https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UPrimitiveComponent
