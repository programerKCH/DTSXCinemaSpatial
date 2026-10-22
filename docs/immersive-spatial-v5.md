# Immersive Spatial DSP v5 (experimental architecture)

Stable baseline: v4.5 Quality Modes, restored on main. Do not modify the stable DSP while testing this branch.

## Goal
One shared DSP for PCM stereo/5.1 sources; maximize positional clarity without altering Cinema Impact, Cinema Strike, or authored LFE. Separate spatial rendering/output backend from audio enhancement.

## Reality of source and output
- A 5.1-channel JUCE VST2 effect is **not** an Atmos/DTS:X object encoder or Windows spatial renderer.
- Existing Height Mode mixes filtered cues into surround PCM; it does not create ceiling channels.
- Implementing moving 3D objects on Windows requires an application/output host using Microsoft's ISpatialAudioClient and ISpatialAudioObjectRenderStream (or compatible spatial output API), not simply changing the VST DSP.
- Dolby Atmos for home theater through supported Windows spatial paths can be represented as spatial objects. DTS:X home theater availability must be verified on the target Windows/HDMI chain. Do not promise identical mode support without a hardware test.

## Incremental plan
1. Keep v4.5 bit-for-bit as the reference; capture matching audio test files and build automated A/B null comparison.
2. Introduce an optional spatial-scene model with source angle, elevation, distance, width, and confidence. Avoid calling heuristic positions 'true source objects'.
3. Source analysis: center coherence/dialogue lock, diffuse ambience, hard-panned transients; enforce stable trajectories with smoothing and no new LFE.
4. Add a **separate** Windows Spatial Sound prototype host; enumerate available static/dynamic object capacity at runtime.
5. Fall back to unchanged 5.1 PCM for unsupported outputs, preserving original channels and bass management.
6. Verify eARC and AVR direct connection separately. Log negotiated format, channels, and object count. Confirm renderer/AVR output rather than assuming it.
7. Compare baseline vs prototype for clipping, transient preservation, dialogue localization, channel separation and subjective immersion.

## First safety rules
- Original mode always remains the clean v4.5 reference.
- Do not mutate stable Impact, Strike, Bass, loudness or limiter coefficients.
- No faux Atmos/DTS:X metadata, no unlicensed stream-encoder claims.
- Never mix synthesized moving objects on top of the unchanged channels at full level (double energy).


## Requested Height Mode behavior (product contract)
- Off: no synthetic elevation layer; keep original channel bed unchanged.
- Virtual Height: derive a restrained, time-aligned height impression from diffuse ambience/moving effects and mix into existing surround PCM channels. Never masquerade as an actual ceiling speaker or alter direct dialogue/localization.
- Real Height: ONLY when an output host has negotiated dedicated height/static spatial channels or supported spatial objects. Route scene elements to the actual height-capable renderer/channel slots (e.g., supported top front/top rear static objects); DO NOT simply add them to surround.
- When Real Height is selected but no capable renderer/height channels are available, display an explicit unsupported/fallback status and use Virtual Height only if opted in. Never silently label virtual surround as real height.
- Height Volume scales the synthesized elevation layer only, independently of Cinema Impact/Strike/Bass.

## Listening reference: Sennheiser AMBEO soundbar immersive mode
Design goal, not reverse engineering: expansive but coherent lateral and vertical ambience, convincing smooth object trajectories, grounded dialogue and unchanged clean v4.5 transient/bass signature. Avoid over-brightness, echo, artificial phase artifacts or duplicate source energy.

## Common Atmos / DTS:X routing policy
- Use a format-neutral scene bus: {azimuth,elevation,distance,spread,confidence,wet_gain} plus stationary multichannel bed.
- Apply the SAME source-analysis engine across outputs. A host adapter interrogates the OS-selected spatial render endpoint instead of assuming a codec from configuration labels.
- When supported, bind dynamic objects through Windows Spatial Sound APIs and static height channel objects where available. Let Windows/spatial provider render to Dolby Atmos for Home Theater or DTS:X for Home Theater as supported; the VST itself must not claim to encode either codec.
- Preserve front/center/LFE integrity. Energy-budget any extracted object against its bed source to avoid loudness doubling. Smooth positions and avoid hard swings across speakers.
- Return exact matched 5.1 PCM on unsupported paths, not an unverified synthetic object bitstream.
- Ensure scene analysis and real-height output do not execute twice if a spatial game is already delivering native positioned objects.

## Acceptance tests
1. Original v4.5 reference null-test against unchanged PCM fallback.
2. Virtual Height/Real Height/Off differ in destination routing, not in Impact/Strike or default bass.
3. Real Height routes to top channels only on genuinely compatible hardware; unsupported status is visible.
4. Repeatable pans, flyover sounds, overhead rain and fixed centered dialogue: object positions stable, no clicking/booming/peak overload.
5. Verify user setup: RTX 5080, Windows 11, Onkyo TX-NR6100, connected display; test eARC and direct-AVR separately for output caps and negotiated format.
