# Citadel Run — implementation and timed local test

Version 1.3.1, November 5, 2026. Based on the user's latest Shooting(1).zip.

## What is implemented

- Five 72-meter combat rooms linked by corridors and four clock gaps; start to
  extraction spans approximately 575 meters along the route axis.
- Ten squads, six enemies each. Future squads are hidden, have collision
  disabled, and do not move or attack. Only the current wave in the current
  sector is enabled.
- Seven visibly distinct procedural clockwork roles, using the project's
  shared material and built-in primitive meshes. The original FPS player is
  unchanged. These are robot variants, not newly imported humanoid assets.
- Shield orientation and charge windows; bomber radius/windup with sight
  cancellation; strafing skirmishers; twin-bolt sentries; two Warden patterns.
- Sector gates use local completion. Final extraction requires all 60 robots
  and all eight checkpoint flags. The nearest-objective marker follows the
  active squad, then the next flag/clock, then final extraction.
- Each new clock parks at its near dock until boarded. Retries at a near-side
  flag reset that clock. Health, magazine, and defeat progress are retained as
  described in README.md.
- Previous daylight fill, readable shadows, numbered flags and floor arrows
  remain. Scenery has a distance-culling limit; only six enemies fight at once.
- Enemy bolts consume one collision and defer destruction briefly, preventing
  duplicate damage and letting other collision callbacks finish.

The new map is L_ClockCampaign. The original L_ClockArena map and all uploaded
art/material/audio assets are preserved in the archive. Existing campaign
maps are refreshed without automatically rebuilding their layout on launch.

## Duration target

The assignment needs at least five minutes. The design targets a first run of
5–8 minutes through sustained play: new tactics, side routes, two squads per
sector, checkpoint breaks and four rides. It does not enforce a minimum time.
A practiced player can be quicker, so the target must be checked locally.

This is a planned first-play pacing budget, not observed data:

| Section including movement/combat | Planned time |
| --- | --- |
| Arrival Yard | 45–70 s |
| Cooling Gardens | 45–70 s |
| Gearworks | 50–80 s |
| Signal Roofs | 45–75 s |
| Clock Citadel and extraction | 60–90 s |
| Four rides and boarding | 55–75 s total |

Ranges overlap with player movement and recovery. Do not present their sum as
a measured completion time.

## Required local test

1. Build with Launch_Windows.bat. Preserve the first compiler/SDK error if it
   fails. Wait for CLOCK CAMPAIGN READY and confirm L_ClockCampaign is open.
2. Check the start and objective: SECTOR 1 / 5, WAVE 1 / 2, zero saved flags,
   and 0 / 60 eliminations. Test the original weapon, reload and aim.
3. Clear the first six enemies. Confirm the next squad becomes visible with
   a warning, while later sectors remain inactive. The gate must stay shut
   until all 12 local robots are cleared.
4. Walk through flag one. Check health/magazine restoration and the green flag.
   The first clock must remain parked while you wait on the solid dock. Step
   completely onto the hand to start its boarding pause and rotation.
5. Ride to 3, enter flag two and verify sector two begins. Repeat all four
   bridges and flags in sequence. Test one fall/retry on each bridge.
6. Test shield guards from the front, rear and during charging. Check the shield
   breaks after two blocked hits and body damage still works. Fire at the head
   and body of each model; verify the shared hit capsule feels reasonable.
7. Step into and out of a bomber's warning area. Moving out or hiding behind
   solid cover must cancel detonation; killing it must count only once.
8. Check skirmisher sideways movement, sentry two-bolt bursts, and the Warden's
   wider five-bolt fan at half armor. Enemies and their projectiles must not
   attack through cover. Check for pending-kill Blueprint errors.
9. Test the galleries, flanks and narrow docks. Check for stuck enemies, hidden
   remaining targets, cover intersections and overlaps when a squad activates.
10. Clear all robots and enter final extraction. Verify victory, displayed time,
    correct elimination count, and Enter replay. Also test pause and retry.
11. Repeat once in a packaged build after the new map has been generated and saved.

## Time the complete run

Play from the initial spawn to extraction without using editor shortcuts.
Record both wall-clock time and the displayed run time; pausing and the short
death return affect them differently. Try one first-time player if possible.

| Run/device | Prior familiarity | Wall time | Displayed time | Retries | Confusion/stalls |
| --- | --- | --- | --- | --- | --- |
| Fill after playing | — | — | — | — | — |
| Fill after playing | — | — | — | — | — |

If a normal first run is under 5:00, expand an encounter or add a meaningful
route objective before submission. Prefer additional placement/tactics over
increasing every enemy's armor or adding an idle timer. If a run feels slow,
look for stuck enemies, unclear directions or repetitive fights before adding
more content. Actual balancing requires these observations.

## Offline evidence

| Check | Result | Limit |
| --- | --- | --- |
| Existing combat/clock/layout checks | 1,673 pass | Standard C++ headers |
| Legacy route/flags/boarding checks | 794 pass | No engine physics |
| Campaign progression, 60 spawn capsules, cover clearance, four docks, shields, burst profiles and windups | 1,655 pass | No engine simulation |
| Existing/new map preparation and failure handling | 13 pass | Editor subsystem stand-ins |
| Python/JSON/shell syntax and source/header consistency | Checked | No native Unreal compile |
| Original Content and input settings | SHA-256 / byte comparison passed | Campaign generated inside Unreal |
| Delivery archive | CRC verified at packaging | No cooked executable |

No Unreal runtime or human playtest has been run here. Visual fidelity,
collisions, audio playback, timing, and performance are unverified locally.

Primary API references checked:
- https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UPrimitiveComponent
- https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UCapsuleComponent
- https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UWorld/SpawnActor

## Setup and encounter polish in 1.3.1

Editor preparation now waits while Play or Simulate is active; a manual attempt
to generate the map during play reports a clear stop-play instruction. The
registered startup callback runs once after assets are ready and play has
stopped. This is covered by setup regression tests.

A new original S_WaveAlert PCM source distinguishes squad arrivals from the
checkpoint-save cue. Existing sound sources were preserved. Squad activation
staggers initial attack cooldowns by spawn position, providing readable warning
windows. The cue is imported automatically during editor preparation.

The new WAV is mono, 16-bit, 22050 Hz, 0.6 seconds, with no clipped samples.
Actual playback, native compilation and the timed route remain local checks.

Startup API reference checked:
https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/UnrealEditorSubsystem
