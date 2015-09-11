#include "codec.h"

void FrameCoder::Init(int numch,int framesz)
{
  numchannels=numch;
  framesize=framesz;

  samples.resize(numchannels);
  error.resize(numchannels);
  temp.resize(numchannels);
  for (int i=0;i<numchannels;i++) {
    samples[i].resize(framesize);
    error[i].resize(framesize);
    temp[i].resize(framesize);
  }
  encoded.resize(numchannels);
  numsamples=0;
}

void FrameCoder::PredictMonoFrame(int ch,int numsamples)
{
  O1Pred myPred;
  const int32_t *src=&(samples[ch][0]);
  int32_t *dst=&(error[ch][0]);

  for (int i=0;i<numsamples;i++) {
    int32_t val=src[i];
    int32_t pred=myPred.Predict();

    int32_t err=val-pred;

    dst[i]=err;
    myPred.Update(val);
  }
}

void FrameCoder::UnpredictMonoFrame(int ch,int numsamples)
{
  O1Pred myPred;
  const int32_t *src=&(error[ch][0]);
  int32_t *dst=&(samples[ch][0]);

  for (int i=0;i<numsamples;i++) {
    int32_t err=src[i];
    int32_t pred=myPred.Predict();

    int32_t val=err+pred;

    dst[i]=val;
    myPred.Update(val);
  }
}


void FrameCoder::EncodeMonoFrame(int ch,int numsamples)
{
  const int32_t *src=&(error[ch][0]);
  BufIO &buf=encoded[ch];
  buf.Reset();

  RangeCoderSH rc(buf);
  rc.Init();

  Rice ricecoder(rc);

  for (int i=0;i<numsamples;i++) ricecoder.Encode(src[i]);
  rc.Stop();
}

void FrameCoder::DecodeMonoFrame(int ch,int numsamples)
{
  int32_t *dst=&(error[ch][0]);
  BufIO &buf=encoded[ch];
  buf.Reset();

  RangeCoderSH rc(buf,1);
  rc.Init();

  Rice ricecoder(rc);

  for (int i=0;i<numsamples;i++) dst[i]=ricecoder.Decode();
  rc.Stop();
}

void FrameCoder::Predict()
{
  for (int ch=0;ch<numchannels;ch++) PredictMonoFrame(ch,numsamples);
}

void FrameCoder::Unpredict()
{
  for (int ch=0;ch<numchannels;ch++) UnpredictMonoFrame(ch,numsamples);
}

void FrameCoder::Encode()
{
  for (int ch=0;ch<numchannels;ch++) EncodeMonoFrame(ch,numsamples);
}

void FrameCoder::Decode()
{
  for (int ch=0;ch<numchannels;ch++) DecodeMonoFrame(ch,numsamples);
}


void FrameCoder::WriteEncoded(AudioFile &fout)
{
  uint8_t buf[4];
  binUtils::put32LH(buf,numsamples);
  fout.file.write(reinterpret_cast<char*>(buf),4);
  cout << "numsamples: " << numsamples << endl;
  cout << " filepos:   " << fout.file.tellg() << endl;
  for (int ch=0;ch<numchannels;ch++) {
    uint32_t blocksize=encoded[ch].GetBufPos();
    binUtils::put32LH(buf,blocksize);
    cout << " blocksize: " << blocksize << endl;
    fout.file.write(reinterpret_cast<char*>(buf),4);
    fout.WriteData(encoded[ch].GetBuf(),blocksize);
  }
}

void FrameCoder::ReadEncoded(AudioFile &fin)
{
  uint8_t buf[4];
  fin.file.read(reinterpret_cast<char*>(buf),4);
  numsamples=binUtils::get32LH(buf);
  //cout << "numsamples: " << numsamples << endl;
  //cout << " filepos:   " << fin.file.tellg() << endl;
  for (int ch=0;ch<numchannels;ch++) {
    fin.file.read(reinterpret_cast<char*>(buf),4);
    uint32_t blocksize=binUtils::get32LH(buf);
    //cout << " blocksize: " << blocksize << endl;
    fin.ReadData(encoded[ch].GetBuf(),blocksize);
  }
}

void Codec::EncodeFile(Wav &myWav,Sac &mySac)
{
  const int numchannels=myWav.getNumChannels();
  framesize=1*myWav.getSampleRate();

  myFrame.Init(numchannels,framesize);

  mySac.WriteHeader(myWav);
  myWav.InitFileBuf(framesize);

  int samplescoded=0;
  int samplestocode=myWav.getNumSamples();
  while (samplestocode>0) {
    int samplesread=myWav.ReadSamples(myFrame.samples,framesize);

    myFrame.SetNumSamples(samplesread);
    myFrame.Predict();
    myFrame.Encode();
    myFrame.WriteEncoded(mySac);

    samplescoded+=samplesread;
    double r=samplescoded*100.0/(double)myWav.getNumSamples();
    cout << "  " << samplescoded << "/" << myWav.getNumSamples() << ":" << setw(6) << miscUtils::ConvertFixed(r,1) << "%\r";

    samplestocode-=samplesread;
  }
  cout << endl;
}

void Codec::DecodeFile(Sac &mySac,Wav &myWav)
{
  const int numchannels=mySac.getNumChannels();
  framesize=1*mySac.getSampleRate();

  myFrame.Init(numchannels,framesize);

  int samplestodecode=mySac.getNumSamples();
  int samplesdecoded=0;

  myWav.InitFileBuf(framesize);
  mySac.UnpackMetaData(myWav);
  myWav.WriteHeader();
  while (samplestodecode>0) {
    myFrame.ReadEncoded(mySac);
    myFrame.Decode();
    myFrame.Unpredict();
    myWav.WriteSamples(myFrame.samples,myFrame.GetNumSamples());
    samplesdecoded+=myFrame.GetNumSamples();

    double r=samplesdecoded*100.0/(double)myWav.getNumSamples();
    cout << "  " << samplesdecoded << "/" << myWav.getNumSamples() << ":" << setw(6) << miscUtils::ConvertFixed(r,1) << "%\r";

    samplestodecode-=myFrame.GetNumSamples();
  }
  myWav.WriteHeader();
  cout << endl;
}


