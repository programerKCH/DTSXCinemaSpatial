#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace immersive {
// Block-level heuristics: avoid claiming that bed PCM exposes original Atmos object IDs.
enum class SceneKind { Quiet, DiffuseAmbience, MovingCandidate, Impact, Uncertain };
struct SceneDecision {
    SceneKind kind=SceneKind::Uncertain;
    float azimuth=0.0f;
    float confidence=0.0f;
    float virtualWet=0.0f;
    bool objectEligible=false;
};
class SceneClassifier {
    float slowPower=0.0f, smoothAz=0.0f;
    int persistentMovement=0;
public:
    void reset() { slowPower=0;smoothAz=0;persistentMovement=0; }
    SceneDecision analyze(const std::array<float,6>& rms, float secondsPerBlock) {
        const float l=rms[0]*rms[0],r=rms[1]*rms[1],c=rms[2]*rms[2];
        const float sub=rms[3]*rms[3],sl=rms[4]*rms[4],sr=rms[5]*rms[5];
        const float total=l+r+c+sub+sl+sr;
        SceneDecision d;
        if(total<1.0e-9f) { persistentMovement=0;d.kind=SceneKind::Quiet;return d; }
        const float dt=std::max(0.001f,std::min(secondsPerBlock,0.5f));
        const float a=1.0f-std::exp(-dt/0.5f);
        slowPower += a*(total-slowPower);
        const float onset=total/(slowPower+1.0e-8f);
        const float surround=(sl+sr)/total;
        const float voice=c/total;
        const float bass=sub/total;
        const float lr=(l+sl)-(r+sr);
        const float candidateAz=75.0f*lr/(l+r+sl+sr+1.0e-8f);
        const float velocity=std::fabs(candidateAz-smoothAz)/dt;
        smoothAz += (1.0f-std::exp(-dt/0.1f))*(candidateAz-smoothAz);
        if ((velocity>12.0f) && (voice<0.25f) && (bass<0.30f) &&
            (std::fabs(candidateAz)>8.0f)) ++persistentMovement;
        else persistentMovement=std::max(0,persistentMovement-1);
        d.azimuth=smoothAz;
        d.confidence=std::max(0.0f,std::min(1.0f,(1.0f-voice)*(1.0f-bass)));
        if (bass>0.32f || (onset>3.0f && total>0.005f)) {
            d.kind=SceneKind::Impact;
        } else if(surround>0.55f && std::fabs(candidateAz)<25.0f) {
            d.kind=SceneKind::DiffuseAmbience;
            d.virtualWet=0.08f*std::min(1.0f,surround);
        } else if(persistentMovement>=3 && d.confidence>0.55f) {
            d.kind=SceneKind::MovingCandidate;
            d.objectEligible=true;
            d.virtualWet=0.025f;
        } else {
            d.kind=SceneKind::Uncertain;
            d.virtualWet=0.0f;
        }
        return d;
    }
};
}
