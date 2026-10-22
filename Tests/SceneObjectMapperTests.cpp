#include "../Source/SceneObjectMapper.h"
#include <cassert>
#include <cmath>
int main() {
    immersive::SceneObjectMapper mapper;
    mapper.reset(48000);
    immersive::ObjectEstimate moving{};
    moving.azimuth=65.0f; moving.elevation=20.0f; moving.confidence=1.0f;
    immersive::Position3D right{};
    for(int i=0;i<100;++i) right=mapper.update(moving,480);
    assert(right.x>0.6f && right.y>0.1f && right.z<0.0f);
    moving.azimuth=-65.0f;
    immersive::Position3D left{};
    for(int i=0;i<100;++i) left=mapper.update(moving,480);
    assert(left.x < -0.6f);
    moving.confidence=0.0f;
    for(int i=0;i<100;++i) left=mapper.update(moving,480);
    assert(std::fabs(left.x)<0.02f && std::fabs(left.y)<0.02f);
    for(float f: {left.x,left.y,left.z}) assert(std::isfinite(f));
}
