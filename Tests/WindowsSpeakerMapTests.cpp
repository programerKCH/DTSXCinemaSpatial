#include "../Source/WindowsSpeakerMap.h"
#include <cassert>
#include <array>
int main() {
    using namespace immersive;
    const EndpointLayout bed{MASK_51_SIDE,6}, ceiling{MASK_512_SIDE_FRONT,8};
    assert(hasExact51(bed) && !hasExact512(bed));
    assert(hasExact512(ceiling) && !hasExact51(ceiling));
    assert(capabilitiesFor(ceiling).hasDiscreteTopFrontPair);
    assert(!capabilitiesFor(bed).hasDiscreteTopFrontPair);
    assert(!capabilitiesFor(ceiling).spatialObjectsAvailable);
    const EndpointLayout wrong{MASK_512_SIDE_FRONT ^ TFR,8};
    assert(!hasExact512(wrong));
    RenderSample sample{};
    for (int i=0;i<8;++i) sample.internal[i]=float(i+1)*0.1f;
    std::array<float,8> interleaved{};
    sample.route=Route::Discrete512;
    assert(mapToInterleaved(sample,ceiling,interleaved));
    for(int i=0;i<8;++i) assert(interleaved[i]==sample.internal[i]);
    assert(!mapToInterleaved(sample,bed,interleaved));
    for(float y:interleaved) assert(y==0.0f);
    sample.route=Route::Bed51;
    assert(mapToInterleaved(sample,bed,interleaved));
    for(int i=0;i<6;++i) assert(interleaved[i]==sample.internal[i]);
    assert(interleaved[6]==0.0f && interleaved[7]==0.0f);
    assert(!mapToInterleaved(sample,ceiling,interleaved));
}
