#include "wav.h"

void Chunks::Append(uint32_t chunkid,uint32_t chunksize,uint8_t *data,uint32_t len)
{
  tChunk chunk;
  chunk.id=chunkid;
  chunk.csize=chunksize;
  if (len) {
    chunk.data.resize(len);
    copy(data,data+len,chunk.data.begin());
  }
  wavchunks.push_back(chunk);
  metadatasize+=(8+len);
}

uint32_t Chunks::StoreMetaData(vector <uint8_t>&data)
{
  data.resize(metadatasize);
  uint32_t ofs=0;
  for (size_t i=0;i<GetNumChunks();i++) {
    const tChunk &wavchunk=wavchunks[i];
    binUtils::put32LH(&data[ofs],wavchunk.id);ofs+=4;
    binUtils::put32LH(&data[ofs],wavchunk.csize);ofs+=4;
    copy(begin(wavchunk.data),end(wavchunk.data),begin(data)+ofs);
    ofs+=wavchunk.data.size();
  }
  return ofs;
}

void Wav::InitReader(int maxframesize)
{
  readbuf.resize(maxframesize*blockalign);
}

int Wav::ReadSamples(vector <vector <int32_t>>&data,int samplestoread)
{
  // read samples
  if (samplestoread>samplesleft) samplestoread=samplesleft;
  int bytestoread=samplestoread*blockalign;
  file.read(reinterpret_cast<char*>(&readbuf[0]),bytestoread);
  int samplesread=(file.gcount()/blockalign);
  samplesleft-=samplesread;
  if (samplesread!=samplestoread) cout << "warning: read over eof\n";

  // decode samples
  int bufptr=0;
  for (int i=0;i<samplesread;i++) {
    for (int k=0;k<getNumChannels();k++) {
        //int sample2=binUtils::cnvU2S(binUtils::get16LH((uint8_t*)(&readbuf[bufptr])),getBitsPerSample());
        int16_t sample=((readbuf[bufptr+1]<<8)|readbuf[bufptr]);bufptr+=2;
        data[k][i]=sample;
    }
  }

  return samplesread;
}

int Wav::ReadHeader()
{
  uint8_t buf[32];
  vector <uint8_t> vbuf;
  uint32_t chunkid,chunksize;

  file.read(reinterpret_cast<char*>(buf),12); // read 'RIFF' chunk descriptor
  chunkid=binUtils::get32LH(buf);
  chunksize=binUtils::get32LH(buf+4);

  // do we have a wave file?
  if (chunkid==0x46464952 && binUtils::get32LH(buf+8)==0x45564157) {

    myChunks.Append(chunkid,chunksize,buf+8,4);

    while (1) {
      file.read(reinterpret_cast<char*>(buf),8);
      if (!file) {cout << "could not read!\n";return 1;};

      chunkid   = binUtils::get32LH(buf);
      chunksize = binUtils::get32LH(buf+4);
      if (chunkid==0x020746d66) { // read 'fmt ' chunk
        if (chunksize!=16) {cout << "warning: invalid fmt-chunk size\n";return 1;}
        else {
            file.read(reinterpret_cast<char*>(buf),16);
            myChunks.Append(chunkid,chunksize,buf,16);

            int audioformat=binUtils::get16LH(buf);
            if (audioformat!=1) {cout << "warning: only PCM Format supported\n";return 1;};
            numchannels=binUtils::get16LH(buf+2);
            samplerate=binUtils::get32LH(buf+4);
            byterate=binUtils::get32LH(buf+8);
            blockalign=binUtils::get16LH(buf+12);
            bitspersample=binUtils::get16LH(buf+14);
            kbps=(samplerate*numchannels*bitspersample)/1000;
        }
      } else if (chunkid==0x61746164) { // 'data' chunk
        myChunks.Append(chunkid,chunksize,buf,0);
        datapos=file.tellg();
        numsamples=chunksize/numchannels/(bitspersample/8);
        samplesleft=numsamples;
        endofdata=datapos+(streampos)chunksize;
        if (endofdata==filesize) {seektodatapos=false;break;} // if data chunk is last chunk, break
        else {
          int64_t pos=file.tellg();
          file.seekg(pos+chunksize);
        }
      } else { // read remaining chunks
        ReadData(vbuf,chunksize);
        myChunks.Append(chunkid,chunksize,&vbuf[0],chunksize);
      }
      if (file.tellg()==getFileSize()) break;
    }
  } else return 1;

  if (verbose) {
    cout << " Number of chunks: " << myChunks.GetNumChunks() << endl;
    for (size_t i=0;i<myChunks.GetNumChunks();i++)
      cout << "  Chunk" << setw(2) << (i+1) << ": '" << binUtils::U322Str(myChunks.GetChunkID(i)) << "' " << myChunks.GetChunkDataSize(i) << " Bytes\n";
    cout << " Metadatasize: " << myChunks.GetMetaDataSize() << " Bytes\n";
  }
  if (seektodatapos) {file.seekg(datapos);seektodatapos=false;};
  return 0;
}
