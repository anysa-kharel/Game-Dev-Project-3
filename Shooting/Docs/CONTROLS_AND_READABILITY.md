# Controls and readability update — 1.4.0

Built from the uploaded Shootingv3.zip on November 6, 2026. This is a presentation
update to the user's tested campaign, preserving the saved levels and FPS assets.

## Before the run

- A full-screen briefing presents movement, aiming, shooting, reloading,
  crouching, jumping, sprinting, pause, checkpoint retry, and the field guide.
- Use the gold START RUN button or Enter. The menu has no time limit.
- Combat, clock motion, checkpoint activation, and the recorded run timer wait
  for confirmation. Movement and firing input are disabled while the menu is open.
- Confirmation is armed on press and gameplay begins after release, preventing
  the start click from being passed through as a gunshot.
- The template ammo widget is hidden during the briefing and restored
  to its prior visibility when play begins.
- The briefing scales as one layout to fit the viewport. F1 keeps the compact
  controls and gameplay guide available during the run, including C to crouch.

Movement/combat controls were read from the uploaded Enhanced Input mapping
`Content/InfimaGames/FreeFPSTemplate/Data/IMC_FPS.uasset`. The template's input
asset is unchanged. P, Enter, F1, and Backspace are the existing native bindings.

## Readability changes

- Checkpoint and exit flags have dark ink faces and larger white lettering.
  Cyan, gold, and green identify status on the edge stripe, cap, and floor pad.
  A flag's face stays dark when its status changes.
- Area headings, route instructions, bridge instructions, and clock numerals
  receive padded dark backplates with narrow accent edges. Width and height are
  measured from the text so the plate follows the label.
- Backplates are decorative, have no collision, and do not cast shadows. Each
  is attached to its sign and updated in place; refreshing does not stack copies.
- Gate status labels have a dark board and centered white text.
- Sign surfaces use a separate unlit material so bright sunlight does not wash
  out their backgrounds. The editor startup script creates it before refreshing
  and saving the existing campaign map.
- Native HUD text gains a dark outline and shadow, helping floating enemy labels
  and objective captions stand out against bright scenery.

## Apply to your project

Follow the root README. The small patch updates `Source`, `Content/Python`,
`Config/DefaultGame.ini`, documentation, and tests. Preserve the rest of your
project, merge rather than deleting folders, and rebuild with Unreal closed.

After startup, wait for `CLOCK CAMPAIGN READY / CONTROLS + READABLE SIGNS`.
The existing L_ClockCampaign map is refreshed and saved inside Unreal. The ZIP
retains the uploaded serialized maps; the new material and visual components are
applied by the editor script and updated native classes. Open the campaign map
before Play if a different level is currently selected.

## Validation performed

- All 217 uploaded .uasset/.umap files and DefaultInput.ini remain byte-for-byte
  identical before Unreal applies the presentation upgrade locally.
- 4,122 existing combat, campaign, layout, and wayfinding checks pass.
- 1,366 additional checks cover waiting for confirmation, held and released
  start inputs, duplicate requests, a short click between ticks, the shared
  contrast palette, and menu/button bounds at 12 viewport sizes.
- 13 editor-bootstrap regressions pass, including preserving existing layouts,
  refusing to edit during Play, and handling failed load/save operations.
- Python syntax and delivery archive integrity are checked during packaging.

The screen layout reference is a code-rendered design reference, not an Unreal
screenshot. Palette ratios are calculated from the configured linear colors;
actual display appearance still needs a check in the engine.

## Check in Unreal after rebuilding

1. Press Play and wait at least 30 seconds. The briefing should remain visible;
   no enemy attacks should occur and the run timer should still start at 00:00.
2. Start once with Enter and once with the mouse button. Holding the confirmation
   should show RELEASE TO START; release should hide the briefing, capture the
   mouse for aiming, and restore the ammo widget without firing a start shot.
3. Confirm WASD, mouse look, Space, Shift, C, both mouse buttons, and R perform
   the advertised actions. Check F1, pause/resume, and checkpoint retry.
4. Inspect a checkpoint flag before and after saving it, and the final exit
   before and after unlocking it. White text should stay on a dark face with
   colored status accents. View checkpoint flags from both sides.
5. Inspect area headings, bridge signs, floor clock numerals, gate text, and
   floating HUD labels in bright light. Check that no plate covers its letters.
6. Stop and start Play, then restart the editor. Confirm sign panels do not
   duplicate, and the existing level layout remains intact.
7. Test the controls at your normal window size and in a packaged build. Prepare
   and save the map/material in the editor before packaging.

No Unreal installation or native target SDK is available in this environment.
Unreal compilation, rendering, input capture, and a packaged build require these
local checks. The previously working version was playtested by the user; that
does not constitute a playtest of this update.
