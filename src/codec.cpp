#include "codec.h"

void FrameCoder::Init(int numch,int framesz)
{
  numchannels=numch;
  framesize=framesz;

  samples.resize(numchannels);
  pred.resize(numchannels);
  error.resize(numchannels);
  temp.resize(numchannels);
  for (int i=0;i<numchannels;i++) {
    samples[i].resize(framesize);
    pred[i].resize(framesize);
    error[i].resize(framesize);
    temp[i].resize(framesize);
  }
  encoded.resize(numchannels);
  numsamples=0;
}

void FrameCoder::PredictMonoFrame(int ch,int numsamples)
{
  O1Pred myPred(31,5);
  LPC myLPC(0.998,16,32);
  const int32_t *src=&(samples[ch][0]);
  int32_t *dst=&(error[ch][0]);

  for (int i=0;i<numsamples;i++) {
    int32_t val=src[i];

    int32_t pred1=myPred.Predict();
    int32_t pred2=myLPC.Predict();

    int32_t err1=val-pred1;
    int32_t err2=err1-pred2;

    dst[i]=err2;

    myLPC.Update(err1);
    myPred.Update(val);
  }
}

void FrameCoder::PredictStereoFrame(int ch0,int ch1,int numsamples)
{
  LPC2 lpc0(0.998,16,4,32);
  NLMS nlms0(0.05,128);

  LPC2 lpc1(0.998,16,4,32);
  NLMS nlms1(0.05,128);

  const int32_t *src0=&(samples[ch0][0]);
  const int32_t *src1=&(samples[ch1][0]);
  int32_t *dst0=&(error[ch0][0]);
  int32_t *dst1=&(error[ch1][0]);
  int32_t last=0;
  for (int i=0;i<numsamples;i++) {

    int32_t va=src0[i];
    int32_t vb=src1[i];

    int32_t p0a=lpc0.Predict(last);
    int32_t e0a=va-p0a;
    int32_t p1a=nlms0.Predict();
    int32_t e1a=e0a-p1a;

    int32_t p0b=lpc1.Predict(va);
    int32_t e0b=vb-p0b;
    int32_t p1b=nlms1.Predict();
    int32_t e1b=e0b-p1b;

    dst0[i]=e1a;
    dst1[i]=e1b;

    nlms0.Update(e0a);
    nlms1.Update(e0b);
    lpc0.Update(va);
    lpc1.Update(vb);
    last=vb;
  }
}

void FrameCoder::UnpredictMonoFrame(int ch,int numsamples)
{
  O1Pred myPred(31,5);
  LPC myLPC(0.998,16,32);

  const int32_t *src=&(error[ch][0]);
  int32_t *dst=&(samples[ch][0]);

  for (int i=0;i<numsamples;i++) {
    int32_t pred1=myPred.Predict();
    int32_t pred2=myLPC.Predict();

    int32_t err1=src[i]+pred2;
    int32_t val=err1+pred1;

    dst[i]=val;

    myLPC.Update(err1);
    myPred.Update(val);
  }
}


void FrameCoder::EncodeMonoFrame(int ch,int numsamples)
{
  const int32_t *src=&(error[ch][0]);
  //const int32_t *sn=&(samples[ch][0]);
  //const int32_t *pr=&(pred[ch][0]);
  BufIO &buf=encoded[ch];
  buf.Reset();

  int maxbpn=0;
  int maxs=0;
  for (int i=0;i<numsamples;i++) {
    int val=src[i];
    if (val<0) val=2*(-val);
    else if (val>0) val=(2*val)-1;
    if (val>maxs) maxs=val;
  }
  maxbpn=MathUtils::iLog2(maxs);

  RangeCoderSH rc(buf);
  rc.Init();

  //Golomb vle(rc);
  //for (int i=0;i<numsamples;i++) vle.Encode(src[i]);
  BPNCoder bc(rc,maxbpn);
  for (int i=0;i<numsamples;i++) bc.Encode(src[i]);
  rc.Stop();
}

void FrameCoder::DecodeMonoFrame(int ch,int numsamples)
{
  int32_t *dst=&(error[ch][0]);
  BufIO &buf=encoded[ch];
  buf.Reset();

  RangeCoderSH rc(buf,1);
  rc.Init();

  Golomb vle(rc);

  for (int i=0;i<numsamples;i++) dst[i]=vle.Decode();
  rc.Stop();
}

void FrameCoder::Predict()
{
  if (numchannels==2) PredictStereoFrame(0,1,numsamples);
  else for (int ch=0;ch<numchannels;ch++) PredictMonoFrame(ch,numsamples);
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
  //cout << "numsamples: " << numsamples << endl;
  //cout << " filepos:   " << fout.file.tellg() << endl;
  for (int ch=0;ch<numchannels;ch++) {
    uint32_t blocksize=encoded[ch].GetBufPos();
    binUtils::put32LH(buf,blocksize);
    //cout << " blocksize: " << blocksize << endl;
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

void Codec::PrintProgress(int samplesprocessed,int totalsamples)
{
  double r=samplesprocessed*100.0/(double)totalsamples;
  cout << "  " << samplesprocessed << "/" << totalsamples << ":" << setw(6) << miscUtils::ConvertFixed(r,1) << "%\r";
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
    PrintProgress(samplescoded,myWav.getNumSamples());
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
    PrintProgress(samplesdecoded,myWav.getNumSamples());
    samplestodecode-=myFrame.GetNumSamples();
  }
  myWav.WriteHeader();
  cout << endl;
}


