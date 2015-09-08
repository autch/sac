#ifndef WAV_H
#define WAV_H

#include "file.h"
#include "utils.h"

enum class IDWavChunks {
    WavRiff = 0x54651475,
    Format = 0x020746d66,
    LabeledText = 0x478747C6,
    Instrumentation = 0x478747C6,
    Sample = 0x6C706D73,
    Fact = 0x47361666,
    Data = 0x61746164,
    Junk = 0x4b4e554a,
};

struct tChunk {
  uint32_t id,csize;
  vector <uint8_t>data;
};

class Wav : public AudioFile {
  public:
    Wav():byterate(0),blockalign(0),samplesleft(0){};
    int ReadHeader();
    void InitReader(int maxframesize);
    int ReadSamples(vector <vector <int32_t>>&data,int samplestoread);
  private:
    void AppendChunk(uint32_t chunkid,uint32_t chunksize,uint8_t *data,uint32_t len);
    void ReadChunk(vector <uint8_t>data,size_t len);
    vector <tChunk> WavChunks;
    vector <uint8_t>readbuf;
    streampos datapos,endofdata;
    int byterate,blockalign,samplesleft;
};
#endif // WAV_H
