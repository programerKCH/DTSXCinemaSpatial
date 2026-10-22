#include "../Source/Float51WavReader.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdint>
static void u16(std::ofstream& o,unsigned v){char b[2]={char(v),char(v>>8)};o.write(b,2);}
static void u32(std::ofstream& o,std::uint32_t v){char b[4]={char(v),char(v>>8),char(v>>16),char(v>>24)};o.write(b,4);}
int main(){
    const std::string file="test-float51.wav";
    {
      std::ofstream o(file,std::ios::binary);
      o.write("RIFF",4);u32(o,36+24+24);o.write("WAVE",4);
      o.write("fmt ",4);u32(o,40);u16(o,0xfffe);u16(o,6);u32(o,48000);
      u32(o,48000*24);u16(o,24);u16(o,32);u16(o,22);
      u16(o,32);u32(o,0x60f);u32(o,3);
      const char guidTail[12]={0,0,16,0,char(0x80),0,0,char(0xaa),0,0x38,char(0x9b),0x71};
      o.write(guidTail,12);
      o.write("data",4);u32(o,24);
      for(unsigned i=0;i<6;++i){float f=float(i+1)*0.125f;std::uint32_t word;
        std::memcpy(&word,&f,4);u32(o,word);}
    }
    immersive::Float51WavReader reader;
    assert(reader.open(file));
    assert(reader.framesRemaining()==1);
    std::array<float,6> x{};
    assert(reader.next(x));
    for(unsigned i=0;i<6;++i) assert(x[i]==float(i+1)*0.125f);
    assert(!reader.next(x));
    std::remove(file.c_str());
}
