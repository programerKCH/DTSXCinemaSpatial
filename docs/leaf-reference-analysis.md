# Dolby Leaf 5.1 reference: observed audio behavior and acceptance criteria

Reference provided by the user locally: Leaf-Dolby-thedigitaltheater.mp4.
The copyrighted video/audio is NOT checked into this repository.

## Verified source
- 60.2917 seconds, 1920x1080 H.264 picture.
- AC-3, 48 kHz, six channels, 5.1(side), NOT a Dolby Atmos object metadata stream.
- FFmpeg channel order interpreted as FL FR FC LFE SL SR.
- Measurements use power (mean squared sample level) over the specified time window, not subjective panning inference.

## Channel-power observations
| Segment | Surround proportion (SL+SR) | Center proportion | LFE proportion |
|---|---:|---:|---:|
| 15-20s | 36.4% | 39.6% | 15.7% |
| 20-35s | 73.7% | 19.0% | ~0% |
| 35-50s | 73.5% | 7.6% | 8.0% |
| 50-60s | ~0.1% | 10.9% | 57.9% |

These numbers characterize the **decoded 5.1 mix**, not original Atmos object trajectories or physical speaker placements. Actual trajectories cannot be reliably inferred from energy alone.

## Changes to development criteria
1. High surround fraction plus low center fraction permits *restrained* ambient-height extraction. Do not treat every rear signal as an overhead object.
2. Strong center/dialogue content must remain anchored. Preserve center signal bit-for-bit in the optional height stage.
3. A sub-heavy event such as 50-60s must not cause phantom elevation or added synthetic bass. Preserve authored LFE bit-for-bit.
4. Real Height: transfer a bounded amount of diffuse surround material to a physically negotiated height path; bed+height should conserve source per channel in the prototype. Virtual Height: use surrounds only.
5. Apply short-term confidence gating and path smoothing before promoting any heuristic to a moving spatial object; avoid moving the soundfield on static ambience and avoid pumping with quiet noise.
6. Compare baseline v4.5 against the prototype using a recorded 5.1 PCM path; unit tests do not replace 5.1.2 speaker testing.
7. Stress tests: rear-heavy ambience, centered content, final LFE transient, silent passages, and mode switching.

Reproduce with:
```
python Tools/analyze_51_reference.py path/to/Leaf-Dolby-thedigitaltheater.mp4
```
Requires ffmpeg, ffprobe and numpy. Do not commit source media.
