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
    void Append(uint32_t chunkid,uint32_t chunksize,uint8_t *data,uint32_t len);
    size_t GetNumChunks() const {return wavchunks.size();};
    uint32_t GetChunkID(int chunk) const {return wavchunks[chunk].id;};
    uint32_t GetChunkSize(int chunk) const {return wavchunks[chunk].csize;};
    size_t GetChunkDataSize(int chunk) const {return wavchunks[chunk].data.size();};
    uint32_t GetMetaDataSize() const {return metadatasize;};
    uint32_t StoreMetaData(vector <uint8_t>&data);
  private:
    vector <tChunk> wavchunks;
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
    vector <uint8_t>readbuf;
    streampos datapos,endofdata;
    int byterate,blockalign,samplesleft;
    bool seektodatapos,verbose;
};
#endif // WAV_H
