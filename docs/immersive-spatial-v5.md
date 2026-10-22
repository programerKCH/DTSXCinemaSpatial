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
