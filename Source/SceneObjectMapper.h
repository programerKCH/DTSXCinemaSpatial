#pragma once
#include "ImmersiveScene.h"
#include <cmath>

namespace immersive {
struct Position3D { float x=0.0f,y=0.0f,z=-1.5f; float confidence=0.0f; };
class SceneObjectMapper {
    float x=0.0f,y=0.0f,z=-1.5f;
    float sampleRate=48000.0f;
public:
    void reset(float rate) { sampleRate=std::max(8000.0f,rate); x=0;y=0;z=-1.5f; }
    Position3D update(const ObjectEstimate& e, unsigned frames) {
        const float confidence=bounded(e.confidence,0.0f,1.0f);
        // No confident localization: settle forward rather than orbiting randomly.
        const float angle=bounded(e.azimuth,-85.0f,85.0f)*0.01745329252f;
        const float elevation=bounded(e.elevation,0.0f,60.0f)*0.01745329252f;
        const float radial=1.5f;
        const float targetX=radial*std::sin(angle)*confidence;
        const float targetY=radial*std::sin(elevation)*confidence;
        const float targetZ=-radial*std::cos(angle)*std::cos(elevation);
        const float amount=1.0f-std::exp(-6.0f*float(frames)/sampleRate);
        x+=amount*(targetX-x);y+=amount*(targetY-y);z+=amount*(targetZ-z);
        return {x,y,z,confidence};
    }
};
} // namespace immersive
