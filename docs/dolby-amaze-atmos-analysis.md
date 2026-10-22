# Dolby Amaze TrueHD+Atmos 7.1-bed reference (user supplied, not redistributed)

## File identification
- 63.50 s, 1920x1038 H.264, Dolby TrueHD 48 kHz, 8-channel decoded PCM.
- FFmpeg stream reports "truehd (Dolby TrueHD + Dolby Atmos)".
- The decoded 7.1 signal is a renderer/decoder-accessible bed, NOT decoded Atmos object coordinates.
- Channel sequence from FFmpeg native 7.1: FL FR FC LFE BL BR SL SR.
- Audio was analyzed in 4-second nonoverlapping windows by mean-squared channel energy.
- Images inspected: Dolby slate, landscape, rainforest, storm, dark forest.

## Grounded audio observations
| Time | Energy distribution | Design relevance |
|---|---|---|
| 0–8s | mostly rear-side surround, low total level | retain quiet diffuse detail |
| 8–12s | center ~84% | dialogue/narration anchor must stay stationary |
| 16–20s | back+side surrounds ~91%, LFE negligible | broad environmental ambience should envelop without making a false moving source |
| 24–32s | center and surround trade prominence; 28–32s ~84% surround | preserve rear-to-side distinctions where source has 7.1 |
| 32–40s | rear+side ~74% (36–40s), LFE ~30%, very high total energy | thunder/storm: no extra LFE and no height-gain clipping |
| 44–48s | side surround ~76%, back ~5% | direction changes require stable, smoothing-based pan |
| 52–56s | front left/right ~62%, side/rear ~26% | preserve a coherent front stage during movement |
| 56–64s | high-level mixed surrounds / front; late LFE | do not pump near ending |

Some channel energy is carried by the soundtrack's music, effects and narration together. These aggregate figures alone cannot prove the bird's exact 3D flight path, overhead rain source angle, or discrete Atmos object trajectories.

## Virtual Height rules derived from this reference
1. Never reposition sustained wide rain/forest ambience as one discrete, fast-moving object. Use a slowly varying diffuseness-driven surround width and decorrelation budget.
2. Transient/bird candidates: require persistence, channel-energy asymmetry, movement over time, and confidence gating before moving a virtual position. Crossfade over dozens of milliseconds, not abrupt hard panning.
3. Overhead rain: low-level, diffuse surround enhancement only on 5.1 hardware; do NOT claim actual elevation or add fake rear LFE.
4. Thunder: preserve LFE and primary transient waveform; automatically reduce spatial wet mix in high-energy scenes to avoid the v4.6/v4.7 overload problem.
5. Keep front and center stable (speech narration at 8-12 seconds) and ensure front-to-surround motion has no perceived channel jump.
6. Real Height requires endpoint capability, not just a toggle. Do not extract Atmos object coordinates from this 7.1 PCM.
7. The user has no physical overhead speakers: prioritize the Virtual Height fallback first; Real Height is a separate, testable output adapter.

## Acceptance test clips
- 8–12s centered narration: no center alteration.
- 16–20s enveloping ambience: smooth, modest surround width; no false flight motion.
- 28–32s strong surround soundfield: no pumping or stereo image collapse.
- 36–40s thunder peak: no new bass, no clipping from the spatial stage.
- 44–56s dominant surround then front: preserved localization transition.

No copyrighted media embedded in the repository. Repeat the analysis from the user's local MKV as needed.
