#include "wav.h"

void Chunks::Append(uint32_t chunkid,uint32_t chunksize,const uint8_t *data,uint32_t len)
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

size_t Chunks::PackMetaData(vector <uint8_t>&data)
{
  data.resize(metadatasize);
  size_t ofs=0;
  for (size_t i=0;i<GetNumChunks();i++) {
    const tChunk &wavchunk=wavchunks[i];
    binUtils::put32LH(&data[ofs],wavchunk.id);ofs+=4;
    binUtils::put32LH(&data[ofs],wavchunk.csize);ofs+=4;
    copy(begin(wavchunk.data),end(wavchunk.data),begin(data)+ofs);
    ofs+=wavchunk.data.size();
  }
  return ofs;
}

size_t Chunks::UnpackMetaData(const vector <uint8_t>&data)
{
  size_t ofs=0;
  while (ofs<data.size()) {
    uint32_t chunkid,chunksize;
    chunkid=binUtils::get32LH(&data[ofs]);ofs+=4;
    chunksize=binUtils::get32LH(&data[ofs]);ofs+=4;
    if (chunkid==0x46464952) {Append(chunkid,chunksize,&data[ofs],4);ofs+=4;}
    else if (chunkid==0x61746164) {Append(chunkid,chunksize,NULL,0);}
    else {Append(chunkid,chunksize,&data[ofs],chunksize);ofs+=chunksize;};
  }
  return ofs;
}

void Wav::InitFileBuf(int maxframesize)
{
  filebuffer.resize(maxframesize*blockalign);
}

int Wav::ReadSamples(vector <vector <int32_t>>&data,int samplestoread)
{
  // read samples
  if (samplestoread>samplesleft) samplestoread=samplesleft;
  int bytestoread=samplestoread*blockalign;
  file.read(reinterpret_cast<char*>(&filebuffer[0]),bytestoread);
  int samplesread=(file.gcount()/blockalign);
  samplesleft-=samplesread;
  if (samplesread!=samplestoread) cout << "warning: read over eof\n";

  // decode samples
  int bufptr=0;
  for (int i=0;i<samplesread;i++) { // unpack samples
    for (int k=0;k<numchannels;k++) {
        //int sample2=binUtils::cnvU2S(binUtils::get16LH((uint8_t*)(&readbuf[bufptr])),getBitsPerSample());
        int16_t sample=((filebuffer[bufptr+1]<<8)|filebuffer[bufptr]);bufptr+=2;
        data[k][i]=sample;
    }
  }

  return samplesread;
}

int Wav::WriteSamples(vector <vector <int32_t>>&data,int samplestowrite)
{
  int bufptr=0;
  for (int i=0;i<samplestowrite;i++) { // pack samples
    for (int k=0;k<numchannels;k++) {
        int16_t sample=data[k][i];
        filebuffer[bufptr]=sample&0xff;
        filebuffer[bufptr+1]=(sample>>8)&0xff;
        bufptr+=2;
    }
  }
  int bytestowrite=samplestowrite*blockalign;
  file.write(reinterpret_cast<char*>(&filebuffer[0]),bytestowrite);
  return 0;
}

int Wav::ReadHeader()
{
  bool seektodatapos=true;
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
        if (chunksize!=16 && chunksize!=18) {cout << "warning: invalid fmt-chunk size\n";return 1;}
        else {
            file.read(reinterpret_cast<char*>(buf),chunksize);
            myChunks.Append(chunkid,chunksize,buf,chunksize);

            int audioformat=binUtils::get16LH(buf);
            if (audioformat!=1) {cout << "warning: only PCM Format supported\n";return 1;};
            numchannels=binUtils::get16LH(buf+2);
            samplerate=binUtils::get32LH(buf+4);
            byterate=binUtils::get32LH(buf+8);
            blockalign=binUtils::get16LH(buf+12);
            bitspersample=binUtils::get16LH(buf+14);
            if (chunksize==18) {
               int cbsize=binUtils::get16LH(buf+16);
               cout << "  WAVE-Ext size: " << cbsize << " Bytes"<<endl;
            }
            kbps=(samplerate*numchannels*bitspersample)/1000;
        }
      } else if (chunkid==0x61746164) { // 'data' chunk
        myChunks.Append(chunkid,chunksize,NULL,0);
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
        uint32_t readsize=chunksize;
        if (chunksize&1) readsize++; // chunkdata is word-aligned, although chunksize may be odd
        ReadData(vbuf,readsize);
        myChunks.Append(chunkid,chunksize,&vbuf[0],readsize);
      }
      if (file.tellg()==getFileSize()) break;
    }
  } else return 1;

  if (verbose) {
    cout << " Number of chunks: " << myChunks.GetNumChunks() << endl;
    for (size_t i=0;i<myChunks.GetNumChunks();i++)
      cout << "  Chunk" << setw(2) << (i+1) << ": '" << binUtils::U322Str(myChunks.GetChunkID(i)) << "' " << myChunks.GetChunkSize(i) << " Bytes\n";
    cout << " Metadatasize: " << myChunks.GetMetaDataSize() << " Bytes\n";
  }
  if (seektodatapos) {file.seekg(datapos);seektodatapos=false;};
  return 0;
}

int Wav::WriteHeader()
{
  uint8_t buf[8];
  while (chunkpos<myChunks.GetNumChunks())
  {
    const Chunks::tChunk &chunk=myChunks.wavchunks[chunkpos++];
    binUtils::put32LH(buf+0,chunk.id);
    binUtils::put32LH(buf+4,chunk.csize);
    file.write((char*)buf,8);
    if (chunk.id==0x61746164) break;
    else {
       if (verbose) cout << " chunk size " << chunk.data.size() << endl;
       WriteData(chunk.data,chunk.data.size());
    }
  }
  return 0;
}

