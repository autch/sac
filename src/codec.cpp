#include "codec.h"

FrameCoder::FrameCoder(int numch,int framesz,int profile)
{
  numchannels=numch;
  framesize=framesz;

  if (profile==0)   {
    baseprofile.Init(1,0);
    baseprofile.Set(0,0.99,0.999,0.996);
  } else if (profile==1) {
    baseprofile.Init(6,1);
    baseprofile.Set(0,0.99,0.9999,0.998);
    baseprofile.Set(1,0.000001,0.001,0.0004);
    baseprofile.Set(2,0.000001,0.001,0.0001);
    baseprofile.Set(3,0.00001,0.01,0.003);
    baseprofile.Set(4,0.0001,0.1 ,0.01);
    baseprofile.Set(5,0.99,0.9999,0.998);
  } else if (profile==2) {
    baseprofile.Init(8,2);
    baseprofile.Set(0,0.98,0.9999,0.997);
    baseprofile.Set(1,0.0000001,0.0001,0.00001);
    baseprofile.Set(2,0.0000001,0.0001,0.000005);

    baseprofile.Set(3,0.000001,0.001,0.0005);
    baseprofile.Set(4,0.000001,0.001,0.0001);
    baseprofile.Set(5,0.0001,0.01 ,0.003);
    baseprofile.Set(6,0.001,0.1   ,0.01);
    baseprofile.Set(7,0.99,0.9999,0.998);
  }

  framestats.resize(numchannels);
  samples.resize(numchannels);
  err0.resize(numchannels);
  err1.resize(numchannels);
  error.resize(numchannels);
  s2u_error.resize(numchannels);
  s2u_error_map.resize(numchannels);
  for (int i=0;i<numchannels;i++) {
    samples[i].resize(framesize);
    err0[i].resize(framesize);
    err1[i].resize(framesize);
    error[i].resize(framesize);
    s2u_error[i].resize(framesize);
    s2u_error_map[i].resize(framesize);
  }
  encoded.resize(numchannels);
  enc_temp1.resize(numchannels);
  enc_temp2.resize(numchannels);
  numsamples=0;
}

/*void FrameCoder::PredictMonoFrame(int ch,int numsamples)
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
}*/




void FrameCoder::AnalyseChannel(int ch,int numsamples)
{
  if (numsamples) {
    int nbits=16;
    int minval=(1<<(nbits-1))-1,maxval=-(1<<(nbits-1));
    int64_t mean=0;
    int32_t *src=&(samples[ch][0]);

    uint32_t ordata=0,xordata=0,anddata=~0;

    for (int i=0;i<numsamples;i++) {
      if (src[i]<minval) minval=src[i];
      if (src[i]>maxval) maxval=src[i];

      xordata |= src[i] ^ -(src[i] & 1);
      anddata &= src[i];
      ordata |= src[i];

      mean+=src[i];
    }
    /*int tshift=0;
    if (!(ordata&1)) {
        while (!(ordata&1)) {
            tshift++;
            ordata>>=1;
        }
    } else if (anddata&1) {
        while (anddata&1) {
            tshift++;
            anddata>>=1;
        }
    } else if (!(xordata&2)) {
        while (!(xordata&2)) {
            tshift++;
            xordata>>=1;
        }
    }*/
    //cout << "total shift: " << tshift << endl;

    mean=mean/numsamples;
    /*if (mean) {
        minval-=mean;
        maxval-=mean;
        for (int i=0;i<numsamples;i++) src[i]-=mean;
    }*/
    framestats[ch].minval=minval;
    framestats[ch].maxval=maxval;
    framestats[ch].mean=mean;
    std::cout << "mean: " << mean << ", min: " << minval << " ,max: " << maxval << std::endl;
  }
}


void FrameCoder::PredictStereoFrame(const SacProfile &profile,int ch0,int ch1,int from,int numsamples)
{
  StereoPredictor *myProfile=nullptr;
  if (profile.type==0) myProfile=new StereoFast(profile.coefs[0].vdef,framestats);
  else if (profile.type==1) myProfile=new StereoNormal(profile,framestats);
  else if (profile.type==2) myProfile=new StereoHigh(profile,framestats);
  //TestProfile myProfile;

  if (myProfile) {
  const int32_t *src0=&(samples[ch0][from]);
  const int32_t *src1=&(samples[ch1][from]);
  int32_t *dst0=&(error[ch0][0]);
  int32_t *dst1=&(error[ch1][0]);

  int32_t emax0=0,emax1=0;
  int32_t emax0_map=0,emax1_map=0;

  for (int i=0;i<numsamples;i++) {
    myProfile->Predict(src0[i],src1[i],dst0[i],dst1[i]);
    myProfile->Update();

    int pred0=src0[i]-dst0[i];
    int pred1=src1[i]-dst1[i];

    int map_a=framestats[0].mymap.Map(pred0,dst0[i]); // remap the error
    int map_b=framestats[1].mymap.Map(pred1,dst1[i]);

    #if 0 //debug code
      int unmap_a=framestats[0].mymap.Unmap(pred0,map_a);
      int unmap_b=framestats[1].mymap.Unmap(pred1,map_b);
      if (unmap_a != dst0[i]) cout << unmap_a << " " << dst0[i] << endl;
      if (unmap_b != dst1[i]) cout << unmap_b << " " << dst1[i] << endl;
    #endif

    int32_t e0=MathUtils::S2U(dst0[i]);
    int32_t e1=MathUtils::S2U(dst1[i]);

    int32_t e0_map=MathUtils::S2U(map_a);
    int32_t e1_map=MathUtils::S2U(map_b);

    s2u_error[ch0][i]=e0;
    s2u_error[ch1][i]=e1;

    s2u_error_map[ch0][i]=e0_map;
    s2u_error_map[ch1][i]=e1_map;

    if (e0>emax0) emax0=e0;
    if (e1>emax1) emax1=e1;

    if (e0_map>emax0_map) emax0_map=e0_map;
    if (e1_map>emax1_map) emax1_map=e1_map;
  }

  framestats[0].maxbpn=MathUtils::iLog2(emax0);
  framestats[1].maxbpn=MathUtils::iLog2(emax1);

  framestats[0].maxbpn_map=MathUtils::iLog2(emax0_map);
  framestats[1].maxbpn_map=MathUtils::iLog2(emax1_map);
  delete myProfile;
  }
}

void FrameCoder::UnpredictStereoFrame(const SacProfile &profile,int ch0,int ch1,int numsamples)
{
  StereoPredictor *myProfile=nullptr;
  if (profile.type==0) myProfile=new StereoFast(profile.coefs[0].vdef,framestats);
  else if (profile.type==1) myProfile=new StereoNormal(profile,framestats);
  else if (profile.type==2) myProfile=new StereoHigh(profile,framestats);

  if (myProfile) {
  const int32_t *src0=&(error[ch0][0]);
  const int32_t *src1=&(error[ch1][0]);
  int32_t *dst0=&(samples[ch0][0]);
  int32_t *dst1=&(samples[ch1][0]);

  for (int i=0;i<numsamples;i++) {
    myProfile->Unpredict(src0[i],src1[i],dst0[i],dst1[i]);
    myProfile->Update();
  }
  delete myProfile;
  }
}

int FrameCoder::EncodeMonoFrame_Normal(int ch,int numsamples,BufIO &buf)
{
  buf.Reset();
  RangeCoderSH rc(buf);
  rc.Init();
  BitplaneCoder bc(rc,framestats[ch].maxbpn,numsamples);
  int32_t *srca=&(s2u_error[ch][0]);
  bc.Encode(srca);
  rc.Stop();
  return buf.GetBufPos();
}

int FrameCoder::EncodeMonoFrame_Mapped(int ch,int numsamples,BufIO &buf)
{
  buf.Reset();

  RangeCoderSH rc(buf);
  rc.Init();

  BitplaneCoder bc(rc,framestats[ch].maxbpn_map,numsamples);

  MapEncoder me(rc,framestats[ch].mymap.usedl,framestats[ch].mymap.usedh);
  me.Encode();
  //cout << "mapsize: " << buf.GetBufPos() << " Bytes\n";

  bc.Encode(&(s2u_error_map[ch][0]));
  rc.Stop();
  return buf.GetBufPos();
}


void FrameCoder::EncodeMonoFrame(int ch,int numsamples)
{
  int size_normal=EncodeMonoFrame_Normal(ch,numsamples,enc_temp1[ch]);
  int size_mapped=EncodeMonoFrame_Mapped(ch,numsamples,enc_temp2[ch]);
  if (size_normal<size_mapped)
  {
    framestats[ch].enc_mapped=false;
    encoded[ch]=enc_temp1[ch];
  } else {
    framestats[ch].enc_mapped=true;
    encoded[ch]=enc_temp2[ch];
  }
}

void FrameCoder::DecodeMonoFrame(int ch,int numsamples)
{
  int32_t *dst=&(error[ch][0]);
  BufIO &buf=encoded[ch];
  buf.Reset();

  RangeCoderSH rc(buf,1);
  rc.Init();
  if (framestats[ch].enc_mapped) {
    framestats[ch].mymap.Reset();
    MapEncoder me(rc,framestats[ch].mymap.usedl,framestats[ch].mymap.usedh);
    me.Decode();
    std::cout << buf.GetBufPos() << std::endl;
  }

  BitplaneCoder bc(rc,framestats[ch].maxbpn,numsamples);
  bc.Decode(dst);
  rc.Stop();
}

class CostMeanRMS : public CostFunction {
  public:
      double Calc(const int32_t *buf,int numsamples)
      {
        if (numsamples) {
          double pow=0.0;
          for (int i=0;i<numsamples;i++) pow+=std::abs(buf[i]);
          return pow/static_cast<double>(numsamples);
        } else return 0.;
      }
};

// estimate number of needed bits with a simple golomb model
class CostGolomb : public CostFunction {
  public:
      CostGolomb():mean_err(0.98){};
      double Calc(const int32_t *buf,int numsamples)
      {
        double nbits=0;
        if (numsamples) {
          for (int i=0;i<numsamples;i++) {
            int32_t val=MathUtils::S2U(buf[i]);
            int m=std::max(static_cast<int>(mean_err.Get()),1);
            int q=val/m;
            //int r=val-q*m;
            nbits+=(q+1);
            if (m>1) {
              int b=ceil(log(m)/log(2));
              nbits+=b;
            }
            mean_err.Update(val);
          }
          return nbits/(double)numsamples;
        } else return 0;
      }
  private:
    RunExp mean_err;
};

double FrameCoder::GetCost(SacProfile &profile,CostFunction *func,int coef,double testval,int start_sample,int samples_to_optimize)
{
  const int32_t *err0=&(error[0][0]);
  const int32_t *err1=&(error[1][0]);

  SacProfile testprofile=profile;
  testprofile.coefs[coef].vdef=testval;
  PredictStereoFrame(testprofile,0,1,start_sample,samples_to_optimize);
  double c1=func->Calc(err0,samples_to_optimize);
  double c2=func->Calc(err1,samples_to_optimize);
  return (c1+c2)/2.;
}

void FrameCoder::Optimize(SacProfile &profile)
{
  int blocksize=8;
  int samples_to_optimize=numsamples/blocksize;
  //cout << numsamples << "->" << samples_to_optimize << " " << endl;
  int start=(numsamples-samples_to_optimize)/2;
  //cout << start << "->" << start+samples_to_optimize << " " << endl;
  const int32_t *err0=&(error[0][0]);
  const int32_t *err1=&(error[1][0]);

  CostGolomb CostFunc;


  /*vector <SacProfile::coef>&coefs=testprofile.coefs;
  int n=static_cast<int>(coefs.size());
  vector <double>dx(n),gx(n);
  for (int i=0;i<n;i++) {
      dx[i]=(coefs[i].vmax-coefs[i].vmin)*0.01;
  }*/

  /*double cost,dwcost,tcost,grad;
  testprofile=profile;
  cost=GetCost(testprofile,&CostFunc,0,coefs[0].vdef,start,samples_to_optimize);
  dwcost=GetCost(testprofile,&CostFunc,0,coefs[0].vdef+dx[0],start,samples_to_optimize);
  grad=(dwcost-cost)/dx[0];

  cout << cost << " " << dwcost << endl;
  double mu=0.01;*/

  //cout << (coefs[0].vdef-mu*grad/fabs(grad)) << endl;
  //profile.coefs[0].vdef=x;

  double best_cost=GetCost(profile,&CostFunc,0,profile.coefs[0].vdef,start,samples_to_optimize);

  //for (int k=0;k<2;k++)
  for (int i=0;i<8;i++)
  {
      double best_x=profile.coefs[i].vdef;
      double x=profile.coefs[i].vmin;
      double xinc=(profile.coefs[i].vmax-profile.coefs[i].vmin)/10.;
      //cout << i << ":" << best_x << " -> ";
      while (x<=profile.coefs[i].vmax)
      {
          double cost=GetCost(profile,&CostFunc,i,x,start,samples_to_optimize);
          //cout << x << " " << cost << " " << best_cost << endl;
          if (cost<best_cost) {
            best_x=x;
            best_cost=cost;
          }
          x+=xinc;
      }
      std::cout << best_x << std::endl;
      profile.coefs[i].vdef=best_x;
  }
}

void FrameCoder::InterChannel(int ch0,int ch1,int numsamples)
{
  int32_t *src0=&(samples[ch0][0]);
  int32_t *src1=&(samples[ch1][0]);
  uint64_t sum[4],score[4];
  sum[0]=sum[1]=sum[2]=sum[3]=0;
  for (int i=0;i<numsamples;i++) {
    int32_t v0=src0[i];
    int32_t v1=src1[i];

    sum[0]+=abs(v0);
    sum[1]+=abs(v1);
    sum[2]+=abs((v0+v1)>>1);
    sum[3]+=abs(v0-v1);
  }
  score[0]=sum[0]+sum[1];
  score[1]=sum[0]+sum[3];
  score[2]=sum[1]+sum[3];
  score[3]=sum[2]+sum[3];

  if (score[3]*1.5 < score[0]) {
    std::cout << "ms\n";
    for (int i=0;i<numsamples;i++) {
     int32_t v0=src0[i];
     int32_t v1=src1[i];
     src0[i]=(v0+v1)>>1;
     src1[i]=v0-v1;

   };
  }
}

void FrameCoder::Predict()
{
  //for (int ch=0;ch<numchannels;ch++) AnalyseChannel(ch,numsamples);
  //AnalyseSpectrum(0,numsamples);
  //for (int ch=0;ch<numchannels;ch++) AnalyseChannel(ch,numsamples);
  //AnalyseSpectrum(1,numsamples);
  for (int ch=0;ch<numchannels;ch++)
  {
    framestats[ch].mymap.Reset();
    framestats[ch].mymap.Analyse(&(samples[ch][0]),numsamples);
  }
  //cout << framestats[0].mymap.Compare(framestats[1].mymap) << endl;

  //SacProfile optprofile=baseprofile;
  //Optimize(optprofile);
  if (numchannels==2) {
    //InterChannel(0,1,numsamples);
    PredictStereoFrame(baseprofile,0,1,0,numsamples);
  }// else for (int ch=0;ch<numchannels;ch++) PredictMonoFrame(ch,numsamples);
}

void FrameCoder::Unpredict()
{
  if (numchannels==2) UnpredictStereoFrame(baseprofile,0,1,numsamples);
  //else for (int ch=0;ch<numchannels;ch++) UnpredictMonoFrame(ch,numsamples);
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
  uint8_t buf[8];
  binUtils::put32LH(buf,numsamples);
  fout.file.write(reinterpret_cast<char*>(buf),4);
  //cout << "numsamples: " << numsamples << endl;
  //cout << " filepos:   " << fout.file.tellg() << endl;
  for (int ch=0;ch<numchannels;ch++) {
    uint32_t blocksize=encoded[ch].GetBufPos();
    binUtils::put32LH(buf,blocksize);
    uint16_t flag=0;
    if (framestats[ch].enc_mapped) {
       flag|=(1<<9);
       flag|=framestats[ch].maxbpn_map;
    } else {
       flag|=framestats[ch].maxbpn;
    }
    binUtils::put16LH(buf+4,flag);

    //cout << " blocksize: " << blocksize << endl;
    fout.file.write(reinterpret_cast<char*>(buf),6);
    fout.WriteData(encoded[ch].GetBuf(),blocksize);
  }
}

void FrameCoder::ReadEncoded(AudioFile &fin)
{
  uint8_t buf[8];
  fin.file.read(reinterpret_cast<char*>(buf),4);
  numsamples=binUtils::get32LH(buf);
  //cout << "numsamples: " << numsamples << endl;
  //cout << " filepos:   " << fin.file.tellg() << endl;
  for (int ch=0;ch<numchannels;ch++) {
    fin.file.read(reinterpret_cast<char*>(buf),6);
    uint32_t blocksize=binUtils::get32LH(buf);
    uint16_t flag=binUtils::get16LH(buf+4);
    if (flag>>9) {
      framestats[ch].enc_mapped=true;
    }
    framestats[ch].maxbpn=flag&0xff;
    fin.ReadData(encoded[ch].GetBuf(),blocksize);
  }
}

void Codec::PrintProgress(int samplesprocessed,int totalsamples)
{
  double r=samplesprocessed*100.0/(double)totalsamples;
  std::cout << "  " << samplesprocessed << "/" << totalsamples << ":" << std::setw(6) << miscUtils::ConvertFixed(r,1) << "%\r";
}

void Codec::EncodeFile(Wav &myWav,Sac &mySac,int profile)
{
  const int numchannels=myWav.getNumChannels();
  framesize=4*myWav.getSampleRate();

  FrameCoder myFrame(numchannels,framesize,profile);

  mySac.SetProfile(profile);
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
  std::cout << std::endl;
}

void Codec::DecodeFile(Sac &mySac,Wav &myWav)
{
  const int numchannels=mySac.getNumChannels();
  framesize=4*mySac.getSampleRate();

  int samplestodecode=mySac.getNumSamples();
  int samplesdecoded=0;

  myWav.InitFileBuf(framesize);
  mySac.UnpackMetaData(myWav);
  myWav.WriteHeader();
  int profile=mySac.GetProfile();
  FrameCoder myFrame(numchannels,framesize,profile);
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
  std::cout << std::endl;
}
