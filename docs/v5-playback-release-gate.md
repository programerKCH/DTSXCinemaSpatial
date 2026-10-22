# Experimental v5: first playback candidate (NOT the finished upmixer)

## What can be tested
`SpatialBedObjectDemo.exe` reads **48 kHz, six-channel IEEE float32 WAVE_FORMAT_EXTENSIBLE, 5.1(side) speaker mask (0x60f)** and attempts to render six static spatial bed objects plus at most one optional moving dynamic object. It does not install a driver, modify Windows spatial settings, hook applications, encode Dolby Atmos/DTS:X, or loop audio back into the endpoint.

### Before playing
- Do not replace the existing stable v4.5 VST DLL.
- CI **compiling successfully is not proof of real Windows endpoint activation**. First check the Windows build status. The executable currently has **no device playback validation**.
- Open a Windows terminal and run without options to view usage. Playback is **opt-in**:
  `SpatialBedObjectDemo.exe --wav-demo "C:\\path\\to\\5.1-side-float48k.wav"`
- Keep the AVR level low on first execution. Only run with a file you have verified meets the format described above.
- Failure to activate the six static object channels is a supported outcome: Windows renderers vary.
- Avoid testing native TrueHD, encoded Atmos or DTS:X bitstreams with this utility; it reads decoded PCM only.

## Release gate before user-facing testing
1. Linux regressions passing for the current commit.
2. Windows compile step passing for the current commit.
3. A physical-device trial confirming the output route, channel order, event-driven buffer semantics, and no loudness or clipping issues.
4. Explicit confirmation that PC-wide capture/driver/APO integration is **not included** in this test build.
5. Decide whether the requested system-wide Atmos/DTS:X workflow is compatible with chosen Windows audio architecture; do not describe the standalone renderer as achieving that goal.

## Current technical limitation
Loopback capture observes a render endpoint; it is not a safe transparent system-wide effects insertion point. A native app Spatial Audio render stream outputs audio supplied by that app, not all third-party audio. Any general-purpose system-wide routing likely requires dedicated virtual-device/driver or supported APO architecture, plus protected and Dolby/DTS spatial-mode restrictions to be evaluated separately. No claim of supported system-wide Dolby/DTS rendering is made.
