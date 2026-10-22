#include "../Source/BedObjectSplitter.h"
#include <cassert>
#include <cmath>
int main(){
    immersive::BedObjectSplitter splitter;
    std::array<float,6> in{{0.8f,-0.6f,0.3f,0.2f,0.15f,0.05f}};
    immersive::SceneDecision move{};
    move.kind=immersive::SceneKind::MovingCandidate;
    move.objectEligible=true;
    for(int i=0;i<10000;++i){
        auto out=splitter.process(in,move,true);
        assert(out.bed[0]==in[0] && out.bed[1]==in[1] &&
               out.bed[2]==in[2] && out.bed[3]==in[3]);
        assert(std::fabs((out.bed[4]+out.bed[5]+out.objectMono)-(in[4]+in[5]))<1.0e-5f);
        assert(std::isfinite(out.objectMono));
    }
    auto noDevice=splitter.process(in,move,false);
    assert(noDevice.bed==in && noDevice.objectMono==0.0f);
    splitter.reset();
    auto noEligible=splitter.process(in,immersive::SceneDecision{},true);
    assert(noEligible.bed==in && noEligible.objectMono==0.0f);
}
