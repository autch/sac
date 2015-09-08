#ifndef WAV_H
#define WAV_H

#include "file.h"
#include "../utils.h"

class Chunks {
  struct tChunk {
    uint32_t id,csize;
    vector <uint8_t>data;
  };
  public:
    Chunks():metadatasize(0){};
    void append(uint32_t chunkid,uint32_t chunksize,uint8_t *data,uint32_t len);
    size_t getNumChunks() const {return WavChunks.size();};
    uint32_t getChunkID(int chunk) const {return WavChunks[chunk].id;};
    uint32_t getChunkSize(int chunk) const {return WavChunks[chunk].csize;};
    size_t getChunkDataSize(int chunk) const {return WavChunks[chunk].data.size();};
    uint32_t getMetaDataSize() const {return metadatasize;};
  private:
    vector <tChunk> WavChunks;
    uint32_t metadatasize;
};

class Wav : public AudioFile {
  public:
    Wav(bool verbose=false)
    :datapos(0),endofdata(0),byterate(0),blockalign(0),samplesleft(0),seektodatapos(true),verbose(verbose){};
    int ReadHeader();
    void InitReader(int maxframesize);
    int ReadSamples(vector <vector <int32_t>>&data,int samplestoread);
    Chunks &getChunks(){return myChunks;};
  private:
    Chunks myChunks;
    void readData(vector <uint8_t>&data,size_t len);
    vector <uint8_t>readbuf;
    streampos datapos,endofdata;
    int byterate,blockalign,samplesleft;
    bool seektodatapos,verbose;
};
#endif // WAV_H
