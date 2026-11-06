# Audio and gold UI — 1.5.0

## Included behavior

The menu and game ambience use separate 2D audio components, with smooth envelope
changes between the briefing and gameplay. Menu playback gain defaults to 0.20;
gameplay defaults to 0.055. The gameplay bed drops to 16% of its normal gain when
an existing effect is triggered, then recovers gently after the focus hold expires.
Both music components use lower voice priority than normal effects. M mutes music
immediately; effect playback gains and original sound assets are unchanged.

The gameplay bed is softer during pause, death recovery, and victory. Music is
stopped and its components destroyed on world shutdown/replay to avoid stacking
tracks between sessions. Menu and gameplay tracks are imported with looping and
Play When Silent enabled, so fading to zero or muting does not break looping.
The mute state is per play session.

Each gate owns a non-autoplay audio component with sphere attenuation: 600-unit
inner radius plus 7,000-unit falloff. It plays S_GateOpen once when the required
sector is cleared and the door begins moving. Its 1.6-second mechanical cue has
an initial latch, motor movement, and a settling sound. Nearby door cues also
lower music. Previous gates do not replay their cue on a checkpoint retry.

## Theme

A shared palette supplies menu, HUD, sign, and checkpoint colors. It matches the
existing start screen's charcoal, muted gold, and warm ivory. Native HUD labels
use the same clean Canvas text helper as the controls screen. The template's ammo
widget is tinted at runtime without editing its binary Blueprint. Enemy type
labels retain their role colors on dark backplates, matching the multicolored bots.

The shared presentation colors do not change CAEnemy role-color functions.
World signs have warm white lettering on dark plates with gold edges. Checkpoint
faces stay charcoal; their text explicitly identifies SAVED or CHECKPOINT. Saved
marker accents are ivory, ordinary checkpoint accents gold, the locked exit bronze,
and the open exit bright gold. Guidance text now refers to the GOLD EXIT.
Floor arrows and non-colliding emissive scenery accents are gold. The existing
lighting and level geometry remain in the uploaded maps; presentation refresh
updates their visual styling locally.

## Files and assets

Three new source WAVs are included under Tools/AudioSource. The existing editor
bootstrap imports them into /Game/ClockArena/Audio before refreshing and saving
the existing campaign. Source WAVs and imported sound assets are separate files;
the supplied ZIP preserves the uploaded .uasset/.umap files byte for byte. The
new imported sound assets are created inside Unreal on the user's computer.
The existing /Game/ClockArena packaging rule includes them when the project is
prepared and saved before packaging.

The original compositions are generated without third-party recordings or samples:
24 seconds of stereo menu music, 32 seconds of stereo ambient music, and a
1.6-second mono gate cue. All use 32 kHz, 16-bit PCM WAV. The optional generator
requires NumPy/SciPy; the ready-to-use WAVs require no generator dependencies.

## Checks completed

- 5,589 independent C++ checks pass, including 101 new checks for menu/game
  fades, combat ducking, repeated shots, mute/unmute, quiet pause/results, gain
  bounds, one-shot gate state, and exact theme colors.
- 20 Python tests pass: 13 prior editor-bootstrap/map-preservation checks and
  seven new import/settings checks. The new tests cover first import, idempotent
  reopening, music looping, silent playback settings, failed import/save, and
  preventing a looping gate cue. These use editor stand-ins.
- All 219 uploaded .uasset/.umap files, the input configuration, and enemy source
  files were compared with the current upload and preserved byte for byte.
- Original native effect call arguments and volume gains were compared and retained.
- WAV headers, channel counts, durations, peak levels, and RMS levels were checked.
  No new WAV clips. The uncompressed gameplay ambience has about -43.6 dBFS RMS
  at the default gain and about -59.5 dBFS RMS at its ducked target, before device,
  master-volume, and platform processing. These are signal calculations, not
  measured listening or in-engine output levels.
- Python syntax, generated-header placement, changed-file scope, and delivery ZIP
  integrity are checked during packaging.

There is no Unreal installation or target-platform SDK in this environment.
The full native module, editor imports, audio device routing, rendered colors,
font output, spatial attenuation, and packaged build require a local test.
Independent logic tests cannot establish perceived loudness or speaker quality.

## Local playtest after rebuilding

1. Wait for TICK TOCK READY / AUDIO + GOLD UI in the Output Log. Confirm the
   three new SoundWave assets exist under ClockArena/Audio and save the project.
2. Press Play and stay on the controls screen for over 24 seconds. Music should
   loop smoothly while the run timer and enemies wait for Start. Test M off/on.
3. Click Start or press Enter. The menu music should fade down and quiet ambience
   should fade up. Shoot, trigger a new wave, take damage, and hear an enemy fire;
   each important cue should remain clear as the ambience drops underneath it.
4. Clear the first sector and watch the gate rise. Confirm one mechanical opening
   cue, audible from the active room, with no repeated cue while it stays open.
5. Save a checkpoint and retry it. Check saved marker text/ivory accents, the next
   gold route marker, and that earlier gate sounds do not repeat. Finish the run
   to check the final gold exit and victory cue.
6. Pause and toggle music. Check that music remains soft, M affects only music,
   and resuming does not start additional copies. Stop/start Play and replay from
   victory to verify cleanup.
7. Inspect HUD, ammo, field guide, flags, instructions, clock numerals, floor
   arrows, and gate status. Check that the seven robot roles remain distinct.
8. Package the prepared project and repeat the audio/UI checks on the target
   machine and speakers. Adjust GameplayMusicVolume if the ambient bed needs tuning.

Primary API documentation checked for implementation:
- https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UGameplayStatics/CreateSound2D
- https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UAudioComponent
- https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/USoundWave
- https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/UMG/UWidgetTree
