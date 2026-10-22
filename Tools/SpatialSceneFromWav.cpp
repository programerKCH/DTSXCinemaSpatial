#include "../Source/Float51WavReader.h"
#include "../Source/SceneObjectMapper.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
    if(argc!=3) {
        std::cerr<<"Usage: SpatialSceneFromWav input-5.1-float.wav positions.csv\n";
        return 2;
    }
    immersive::Float51WavReader reader;
    if(!reader.open(argv[1])) {
        std::cerr<<"Input must be 48 kHz six-channel 32-bit float WAVE_FORMAT_EXTENSIBLE "
                    "with 5.1(side) channel mask.\n";
        return 3;
    }
    std::ofstream csv(argv[2],std::ios::trunc);
    if(!csv) return 4;
    csv<<"time_seconds,azimuth_deg,elevation_heuristic_deg,confidence,x,y,z\n";
    immersive::SceneEngine scene;
    immersive::SceneObjectMapper mapper;
    scene.reset(48000);mapper.reset(48000);
    std::array<float,6> sample{};
    std::uint64_t count=0;
    while(reader.next(sample)) {
        const auto frame=scene.process(sample,immersive::HeightMode::Off,false);
        ++count;
        if(count%480==0) {
            const auto xyz=mapper.update(frame.scene,480);
            csv<<std::fixed<<std::setprecision(6)<<double(count)/48000.0<<","
               <<frame.scene.azimuth<<","<<frame.scene.elevation<<","
               <<frame.scene.confidence<<","<<xyz.x<<","<<xyz.y<<","<<xyz.z<<"\n";
        }
    }
    if(!csv) return 5;
    std::cout<<"Exported "<<count<<" PCM frames of heuristic positions; "
             <<"not Atmos object metadata.\n";
    return count ? 0 : 6;
}
