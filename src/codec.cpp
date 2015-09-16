#include "codec.h"
#include "pred/lpc.h"
#include "pred/nlms.h"

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


class StereoProfile : public StereoProcess {
  public:
    StereoProfile(int profile=0)
    :pn(0)
    {
      if (profile==0) {
        pn=2;
        p.resize(pn);
        p[0]=new StereoFromMono(new O1Pred(28,5),new O1Pred(28,5));
        p[1]=new StereoFromStereo(new LPC2(0.998,16,4,32),new LPC2(0.998,16,4,32));
      } else if (profile==1) {
        pn=3;
        p.resize(pn);
        p[0]=new StereoFromMono(new O1Pred(28,5),new O1Pred(28,5));
        p[1]=new StereoFromStereo(new LPC2(0.998,16,4,32),new LPC2(0.998,16,4,32));
        p[2]=new StereoFromStereo(new NLMS2(0.05,256,32),new NLMS2(0.05,256,32));
      }
    }
    ~StereoProfile() {
      for (int i=0;i<pn;i++) delete p[i];
    }
    void Predict(int32_t val0,int32_t val1,int32_t &err0,int32_t &err1)
    {
      for (int i=0;i<pn;i++) {
        p[i]->Predict(val0,val1,err0,err1);
        val0=err0;
        val1=err1;
      }
    }
    void Unpredict(int32_t err0,int32_t err1,int32_t &val0,int32_t &val1)
    {
      for (int i=pn-1;i>=0;i--) {
        p[i]->Unpredict(err0,err1,val0,val1);
        err0=val0;
        err1=val1;
      }
    }
    void Update()
    {
      for (size_t i=0;i<p.size();i++) p[i]->Update();
    }
  private:
    vector <StereoProcess*> p;
    int pn;
};

void FrameCoder::PredictStereoFrame(int ch0,int ch1,int numsamples)
{
  StereoProfile myProfile(1);

  const int32_t *src0=&(samples[ch0][0]);
  const int32_t *src1=&(samples[ch1][0]);
  int32_t *dst0=&(error[ch0][0]);
  int32_t *dst1=&(error[ch1][0]);

  for (int i=0;i<numsamples;i++) {
    myProfile.Predict(src0[i],src1[i],dst0[i],dst1[i]);
    myProfile.Update();
  }
}

void FrameCoder::UnpredictStereoFrame(int ch0,int ch1,int numsamples)
{
  StereoProfile myProfile(1);

  const int32_t *src0=&(error[ch0][0]);
  const int32_t *src1=&(error[ch1][0]);
  int32_t *dst0=&(samples[ch0][0]);
  int32_t *dst1=&(samples[ch1][0]);

  for (int i=0;i<numsamples;i++) {
    myProfile.Unpredict(src0[i],src1[i],dst0[i],dst1[i]);
    myProfile.Update();
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

  //Golomb vle(rc);

  BPNCoder bc(rc,0);
  for (int i=0;i<numsamples;i++) dst[i]=bc.Decode();
  //for (int i=0;i<numsamples;i++) dst[i]=vle.Decode();
  rc.Stop();
}

void FrameCoder::Predict()
{
  if (numchannels==2) PredictStereoFrame(0,1,numsamples);
  else for (int ch=0;ch<numchannels;ch++) PredictMonoFrame(ch,numsamples);
}

void FrameCoder::Unpredict()
{
  if (numchannels==2) UnpredictStereoFrame(0,1,numsamples);
  else for (int ch=0;ch<numchannels;ch++) UnpredictMonoFrame(ch,numsamples);
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
  framesize=4*myWav.getSampleRate();

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
  framesize=4*mySac.getSampleRate();

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


