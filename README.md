# DTSXCinemaSpatial

Experimental HDMI spatial-audio upmixer for a Windows PC connected to an Onkyo TX-NR6100.

## Goals
- Preserve native 5.1/7.1 channel placement when supplied.
- For stereo input, offer controlled upmixing with front clarity and stable directional placement.
- Provide configurable Front, Center, Surround, Sub, Height, Cinema Impact, and roll-off controls.
- Keep AVR bass management as the default: avoid duplicating bass redirection or blindly boosting LFE.
- Build transient punch without pumping, sustained bass boom, clipping, or suppressing normal dynamics.
- Prototype psychoacoustic virtual height and optionally Windows Spatial Audio objects **only after device/API capability checks**.
- Preserve existing OpticalCinemaUpmixer v15 Stable and MagicSpatial unchanged.

## Technical constraints
- HDMI bandwidth supports multichannel uncompressed audio but does not increase acoustic SPL on its own.
- Windows DTS:X for Home Theater, Onkyo DTS Virtual:X and DTS Neural:X are separate stages; a VST cannot force the receiver to enable a listening mode.
- A virtual height cue mixed into ordinary PCM is not itself a DTS:X object. Spatial Audio object output requires a distinct, verified Windows API path.
- A 4.1 physical speaker configuration has no physical height or center speaker.

## Workflow policy
Future build distributions should require replacing only `.github/workflows/build-custom.yml`. No untested binary or assumed DTS:X support should be labeled production-ready.

## Status
Initial design baseline; **no DSP DLL implemented or compiled yet**.
