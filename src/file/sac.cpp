#include "sac.h"

int Sac::WriteHeader(Wav &myWav)
{
  Chunks &myChunks=myWav.GetChunks();
  const uint32_t metadatasize=myChunks.GetMetaDataSize();
  uint8_t buf[32];
  vector <uint8_t>metadata;
  buf[0]='S';
  buf[1]='A';
  buf[2]='C';
  buf[3]='2';
  binUtils::put16LH(buf+4,numchannels);
  binUtils::put32LH(buf+6,samplerate);
  binUtils::put16LH(buf+10,bitspersample);
  binUtils::put32LH(buf+12,numsamples);
  binUtils::put32LH(buf+16,0);
  binUtils::put32LH(buf+20,metadatasize);
  file.write((char*)buf,24);
  if (myChunks.PackMetaData(metadata)!=metadatasize) cerr << "  warning: metadatasize mismatch\n";
  WriteData(metadata,metadatasize);
  return 0;
}

int Sac::UnpackMetaData(Wav &myWav)
{
  size_t unpackedbytes=myWav.GetChunks().UnpackMetaData(metadata);
  if (metadatasize!=unpackedbytes) {cerr << "  warning: unpackmetadata mismatch\n";return 1;}
  else return 0;
}

int Sac::ReadHeader()
{
  uint8_t buf[32];
  file.read((char*)buf,24);
  if (buf[0]=='S' && buf[1]=='A' && buf[2]=='C' && buf[3]=='2') {
    numchannels=binUtils::get16LH(buf+4);
    samplerate=binUtils::get32LH(buf+6);
    bitspersample=binUtils::get16LH(buf+10);
    numsamples=binUtils::get32LH(buf+12);
    metadatasize=binUtils::get32LH(buf+20);
    ReadData(metadata,metadatasize);

    return 0;
  } else return 1;
}
