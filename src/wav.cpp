#include "wav.h"

void Wav::AppendChunk(uint32_t chunkid,uint32_t chunksize,uint8_t *data,uint32_t len)
{
  tChunk chunk;
  chunk.id=chunkid;
  chunk.csize=chunksize;
  if (len) {
    chunk.data.resize(len);
    miscUtils::CpyVecCh(chunk.data,data,len);
  }
  WavChunks.push_back(chunk);
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
  file.read((char*)(&readbuf[0]),bytestoread);
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

void Wav::ReadChunk(vector <uint8_t>data,size_t len)
{
  if (data.size()<len) data.resize(len);
  file.read((char*)(&data[0]),len);
}

int Wav::ReadHeader()
{
  uint8_t buf[32];
  uint32_t chunkid,chunksize;

  file.read((char*)buf,12); // read 'RIFF' chunk descriptor
  chunkid=binUtils::get32LH(buf);
  chunksize=binUtils::get32LH(buf+4);

  // do we have a wave file?
  if (chunkid==0x46464952 && binUtils::get32LH(buf+8)==0x45564157) {

    AppendChunk(chunkid,chunksize,buf+8,4);

    while (1) {
      file.read((char*)buf,8);
      if (!file) {cout << "could not read!\n";return 1;};

      chunkid   = binUtils::get32LH(buf);
      chunksize = binUtils::get32LH(buf+4);
      cout << "reading chunk: '" << buf[0] << buf[1] << buf[2] << buf[3] << "' (" << chunksize << "): ";
      if (chunkid==0x020746d66) {
        if (chunksize!=16) {cout << "warning: invalid fmt-chunk size\n";return 1;}
        else {
            cout << "ok" << endl;
            file.read((char*)buf,16);
            AppendChunk(chunkid,chunksize,buf,16);
            int audioformat=binUtils::get16LH(buf);
            if (audioformat!=1) {cout << "warning: only PCM Format supported\n";return 1;};
            numchannels=binUtils::get16LH(buf+2);
            samplerate=binUtils::get32LH(buf+4);
            byterate=binUtils::get32LH(buf+8);
            blockalign=binUtils::get16LH(buf+12);
            bitspersample=binUtils::get16LH(buf+14);
            kbps=(samplerate*numchannels*bitspersample)/1000;
            /*cout << numchannels << endl;
            cout << samplerate << endl;
            cout << kbps << endl;
            cout << byterate << endl;
            cout << blockalign << endl;
            cout << bitspersample << endl;*/
        }
      } else if (chunkid==0x61746164) {
        cout << endl;
        AppendChunk(chunkid,chunksize,buf,0);
        datapos=file.tellg();
        numsamples=chunksize/numchannels/(bitspersample/8);
        samplesleft=numsamples;
        endofdata=datapos+(streampos)chunksize;
        if (endofdata==filesize) break;
        else {
          cout << "skipping\n";
          int64_t pos=file.tellg();
          file.seekg(pos+chunksize);
        }
      } else {
        cout << "skipping\n";
        int64_t pos=file.tellg();
        file.seekg(pos+chunksize);
      }
      if (file.tellg()==getFileSize()) break;
    }
  } else return 1;
  cout << "number of chunks: " << WavChunks.size() << endl;
  for (size_t i=0;i<WavChunks.size();i++) {
    cout << " chunk: " << setw(2) << (i+1) << ": " << WavChunks[i].data.size() << endl;
  }
  return 0;
}
