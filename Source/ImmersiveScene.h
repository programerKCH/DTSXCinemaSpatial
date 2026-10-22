#pragma once
#include <algorithm>
#include <array>
#include <cmath>

// Experimental source-independent spatial scene analysis. No codec encoding.
namespace immersive {
enum class HeightMode { Off, Virtual, Real };
inline float clamp01(float x) { return std::max(0.0f, std::min(1.0f, x)); }
inline float bounded(float x, float lo, float hi) { return std::max(lo, std::min(hi, x)); }
struct ObjectEstimate {
    float azimuth = 0.0f;     // degrees: left negative
    float elevation = 0.0f;   // heuristic, NOT authored metadata
    float confidence = 0.0f;
    float motion = 0.0f;
};
struct Frame {
    std::array<float, 6> channels{}; // L,R,C,LFE,SL,SR
    std::array<float, 2> height{}; // top left/right bus, only valid when negotiated
    ObjectEstimate scene{};
    bool realHeightAvailable = false;
    bool realHeightActive = false;
};
class SceneEngine {
    float sampleRate = 48000.0f;
    float leftEnergy = 0, rightEnergy = 0, centerEnergy = 0;
    float surroundEnergy = 0, previousAzimuth = 0, smoothAzimuth = 0;
    float movement = 0, virtualEnvelope = 0;
    float heightLowL = 0, heightLowR = 0;
    float slowSurroundPower = 0;
    float surroundLeftEnergy = 0, surroundRightEnergy = 0;
    float surroundPan = 0;
public:
    void reset(float rate) {
        sampleRate = std::max(rate, 8000.0f);
        leftEnergy = rightEnergy = centerEnergy = surroundEnergy = 0;
        previousAzimuth = smoothAzimuth = movement = virtualEnvelope = 0;
        heightLowL = heightLowR = slowSurroundPower = 0;
        surroundLeftEnergy = surroundRightEnergy = surroundPan = 0;
    }
    Frame process(const std::array<float, 6>& input, HeightMode mode,
                  bool spatialRendererHasHeight, float amount = 1.0f) {
        Frame result;
        result.channels = input; // bit-for-bit fallback for Off and unsupported Real
        result.realHeightAvailable = spatialRendererHasHeight;
        const auto smooth = [this](float hz) { return 1.0f - std::exp(-6.2831853f * hz / sampleRate); };
        const float energyStep = smooth(18.0f), directionStep = smooth(7.0f);
        leftEnergy += energyStep * (input[0]*input[0] - leftEnergy);
        rightEnergy += energyStep * (input[1]*input[1] - rightEnergy);
        centerEnergy += energyStep * (input[2]*input[2] - centerEnergy);
        const float ambient = 0.5f * (input[4]*input[4] + input[5]*input[5]);
        surroundEnergy += energyStep * (ambient - surroundEnergy);
        surroundLeftEnergy += energyStep * (input[4]*input[4] - surroundLeftEnergy);
        surroundRightEnergy += energyStep * (input[5]*input[5] - surroundRightEnergy);
        // Rear-channel direction helps virtual fly-bys continue across the listener.
        const float rearTotal = surroundLeftEnergy + surroundRightEnergy;
        const float rearPanTarget = (surroundRightEnergy - surroundLeftEnergy)/(rearTotal + 1.0e-8f);
        surroundPan += directionStep * (rearPanTarget - surroundPan);
        // Slow scene loudness reference: duck artificial ambience during sudden loud effects.
        slowSurroundPower += smooth(1.0f) * (ambient - slowSurroundPower);
        const float transientRatio = ambient / (slowSurroundPower + 1.0e-7f);
        const float transientDuck = 1.0f / (1.0f + 0.15f * std::max(0.0f, transientRatio - 2.0f));
        const float total = leftEnergy + rightEnergy + centerEnergy + surroundEnergy + 1.0e-8f;
        const float frontTotal = leftEnergy + rightEnergy + 1.0e-8f;
        const float frontPan = (rightEnergy - leftEnergy)/frontTotal;
        const float rearWeight = bounded(surroundEnergy/total,0.0f,1.0f);
        const float targetAz = 70.0f * ((1.0f-rearWeight)*frontPan + rearWeight*surroundPan);
        smoothAzimuth += directionStep * (targetAz - smoothAzimuth);
        movement += smooth(5.0f) * (std::abs(smoothAzimuth - previousAzimuth) - movement);
        previousAzimuth = smoothAzimuth;
        result.scene.azimuth = smoothAzimuth;
        result.scene.confidence = bounded((leftEnergy + rightEnergy) / total * (1.0f - centerEnergy / total), 0.0f, 1.0f);
        result.scene.motion = bounded(movement * 0.1f, 0.0f, 1.0f);
        const float diffuse = bounded(surroundEnergy / total, 0.0f, 1.0f);
        result.scene.elevation = 55.0f * diffuse; // proxy only; no true source location
        if (mode == HeightMode::Virtual) {
            // Virtual cue: subtle, source-derived, diffuse surround ONLY. No dialogue/LFE changes.
            const float target = bounded(amount, 0.0f, 1.0f) * diffuse;
            virtualEnvelope += smooth(3.0f) * (target - virtualEnvelope);
            const float width = bounded(0.08f * virtualEnvelope * transientDuck, 0.0f, 0.08f);
            // Preserve the original rear-channel sum (mono ambience and bass
            // balance) while widening only the difference signal. Bound peaks
            // conservatively; the main v4.5 DSP is not modified.
            const float side = 0.5f * (input[4] - input[5]);
            // Subtle directional modulation of the rear difference signal:
            // never invent a new sound or alter its mono sum.
            const float directionCue = 0.35f * surroundPan * virtualEnvelope;
            const float delta = (width + directionCue * 0.015f) * side;
            const float peak = std::max(std::fabs(input[4]), std::fabs(input[5]));
            const float headroom = std::max(0.0f, 0.98f - peak);
            const float safeDelta = bounded(delta, -headroom, headroom);
            result.channels[4] = input[4] + safeDelta;
            result.channels[5] = input[5] - safeDelta;
        } else if (mode == HeightMode::Real && spatialRendererHasHeight) {
            // Extract diffuse surround highs into a separate height bus (5.1.2 prototype).
            // Center and LFE remain untouched. Do not amplify the bed when height is active.
            const float alpha = smooth(650.0f);
            heightLowL += alpha * (input[4] - heightLowL);
            heightLowR += alpha * (input[5] - heightLowR);
            const float wet = bounded(amount, 0.0f, 1.0f) * 0.22f * diffuse * transientDuck;
            // No ghost height when the authored surround channel has fallen silent.
            const float extractedL = input[4] == 0.0f ? 0.0f : wet * (input[4] - heightLowL);
            const float extractedR = input[5] == 0.0f ? 0.0f : wet * (input[5] - heightLowR);
            result.height = { extractedL, extractedR };
            result.channels[4] -= extractedL;
            result.channels[5] -= extractedR;
            result.realHeightActive = true;
        }
        return result;
    }
};
} // namespace immersive
