#include "sac.h"

int Sac::WriteHeader()
{
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
  if (myChunks.StoreMetaData(metadata)!=metadatasize) cerr << "  warning: metadatasize mismatch\n";
  file.write((char*)(&metadata[0]),metadatasize);
  return 0;
}
