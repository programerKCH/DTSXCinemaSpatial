#include "../Source/SceneClassifier.h"
#include <array>
#include <cassert>
#include <cmath>
int main(){
 using namespace immersive;
 SceneClassifier c;
 auto quiet=c.analyze({{0,0,0,0,0,0}},0.01f);
 assert(quiet.kind==SceneKind::Quiet && !quiet.objectEligible);
 c.reset();
 SceneDecision ambience{};
 for(int i=0;i<50;++i) ambience=c.analyze({{0.01f,0.01f,0.01f,0,0.22f,0.22f}},0.01f);
 assert(ambience.kind==SceneKind::DiffuseAmbience && ambience.virtualWet>0);
 c.reset();
 SceneDecision impact{};
 for(int i=0;i<40;++i) impact=c.analyze({{0.05f,0.05f,0.02f,0.7f,0.2f,0.2f}},0.01f);
 assert(impact.kind==SceneKind::Impact && impact.virtualWet==0);
 c.reset();
 SceneDecision dialogue{};
 for(int i=0;i<40;++i) dialogue=c.analyze({{0.03f,0.03f,0.5f,0,0.03f,0.03f}},0.01f);
 assert(!dialogue.objectEligible && dialogue.virtualWet==0);
}
