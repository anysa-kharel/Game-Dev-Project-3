# Lighting and checkpoint update — 1.2.0

Updated November 5, 2026 from the user's latest Shooting.zip.

## Basis and preservation

The uploaded project identifies itself as 1.0.0 and contains a saved
L_ClockArena.umap. This update edits that supplied version directly. It does
not replace it with a previous generated project or import unrelated menu/UI
changes from earlier revisions.

Original map and art files are byte-preserved in the archive. Editor preparation
opens the existing arena, refreshes lighting and navigation, and saves it. Cover,
enemy placement, clock geometry settings, and gameplay actors are retained.
Only the original two enormous decorative black backdrop walls are removed.
The new gold route-arrow actors can be regenerated independently.

## Lighting

The old map has two directional lights, no ambient skylight, dark materials,
and a preparation script that skips existing maps. With forward rendering, a
second directional light is not a reliable ambient fill.

The update uses one managed directional sun with forward shading priority 10.
Its component is made Movable before any rotation. A daylight atmosphere and
Movable skylight provide sky fill. The lower hemisphere is a neutral light
color, and the skylight does not cast shadow occlusion under the galleries.

An unbounded post-process volume holds exposure constant, reduces occlusion,
removes the dark vignette, and restrains bloom. Lighter base metal colors and
the new M_ArenaDaylight material add a small constant ambient contribution
(Tint * 0.35) alongside existing glow. This deliberately favors visibility over
strict physical lighting. Direct sun shadows remain to show depth.

The original M_Arena asset is retained. Native tinting has an ambient fallback
until editor preparation creates M_ArenaDaylight. Decorative non-solid scenery
no longer casts large shadows.

## Checkpoints and route

Each existing ACAZone gains a 7-meter pole, flag, colored floor pad, and labels
on both sides. Its original trigger dimensions and checkpoint/exit behavior
remain in place. Flag components are nonblocking.

- Cyan numbered flags mark checkpoints. Saved flags turn green and read SAVED.
- The exit flag is gold and LOCKED while any robot remains, then green and GO HERE.
- Gold arrows are placed only on walkable ground, outside configured cover.
- HUD route progress and named waypoints remain visible. Targets behind the
  player get a TURN BACK edge cue.
- Boarding guidance waits on solid ground when the clock is away, then moves
  its target fully beyond the floor edge onto the aligned hand.
- Crossing early cannot hide unfinished yard enemies; return guidance explains
  the ride back before advancing to extraction.

## Checks run here

| Check | Result | Limit |
| --- | --- | --- |
| Existing combat, health, clock and layout checks | 1,673 pass | Standard C++ only |
| Route, flag state, boarding clearance, full-turn bearing and arrow footprint checks | 794 pass | No Unreal collision/rendering |
| Existing-map and new-map preparation regressions | 8 pass | Uses editor subsystem stand-ins |
| Python/JSON/shell syntax, headers, includes and refresh ordering | Checked | No Unreal headers installed |
| Original Content and input preservation | Verified against the uploaded ZIP | Visual upgrades occur inside Unreal |
| Download archive | CRC checked during packaging | Native compilation still required |

Reproduce the offline tests with `bash Tests/run_rules_tests.sh`.

## Local playtest

1. Build with Launch_Windows.bat and wait for the daylight/flags ready message.
2. Confirm both yards, cover faces, the clock, and space under each gallery are
   clearly visible. Check for competing-light or mobility warnings.
3. Walk through each checkpoint. Verify its pad and flag turn green, its label
   says SAVED, and the HUD progress updates. Die/retry and check respawn.
4. Face away from the next objective and verify the edge arrow and TURN BACK cue.
5. Approach the near dock with the hand away. The marker should stay on solid
   ground; when the hand aligns, it should guide you past the edge onto it.
6. Ride to the far dock, step off, and enter flag two. Test the reverse crossing
   once with yard enemies left alive.
7. Confirm the exit stays LOCKED while enemies remain, turns green after all 12
   are defeated, and walking into the extraction pad shows victory.
8. Restart the editor once. Check one managed sun, two checkpoint flags, one
   exit flag, and 18 three-part gold floor arrows; the upgrade should not pile
   up duplicate flags or arrows. Save before packaging and repeat in a build.

Unreal compilation and these visual/gameplay tests have not been run here.
No engine screenshots, frame rates, or successful local playtests are claimed.

Primary Epic API references checked:

- https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UDirectionalLightComponent
- https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/USkyLightComponent
- https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/ASkyAtmosphere
- https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/FPostProcessSettings
- https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UTextRenderComponent
