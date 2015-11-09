#include "codec.h"
#include "pred/lpc.h"
#include "pred/nlms.h"
#include "pred/bias.h"
#include "pred/rbf.h"

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
    baseprofile.Set(1,0,1000,500);
    //baseprofile.Set(0,0.1,300,1);
    baseprofile.Set(2,0.00005,0.001,0.0003);
    baseprofile.Set(3,0.00005,0.001,0.0001);
    baseprofile.Set(4,0.0001,0.01 ,0.003);
    baseprofile.Set(5,0.001,0.1   ,0.01);
  } else if (profile==2) {
    baseprofile.Init(5,2);
    baseprofile.Set(0,0.99,0.999,0.997);
    baseprofile.Set(1,0.0001,0.005,0.00005);
    baseprofile.Set(2,0.0001,0.005,0.0005);
    baseprofile.Set(3,0.00001,0.001,0.0005);
    baseprofile.Set(4,0.000001,0.0001,0.0001);
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

// Fast Profile
// single stage OLS predictor with fixed order
class StereoProfileFast : public StereoProcess {
  public:
    StereoProfileFast(double alpha,vector <FrameCoder::FrameStats> &stats)
    :lpa(alpha,16,8,32),lpb(alpha,16,8,32),
    stats(stats)
    {
      e[0]=e[1]=e[2]=e[3]=0;
      n[0]=n[1]=n[2]=n[3]=0;
    };
    void Predict(int32_t val0,int32_t val1,int32_t &err0,int32_t &err1)
    {
       double p1a=lpa.Predict(e[1]);

       e[0]=val0;
       e[1]=val1;
       double p2a=lpb.Predict(e[0]);
       e[2]=val0-p1a;
       e[3]=val1-p2a;

       int32_t ip1=round(p1a);
       int32_t ip2=round(p2a);

       err0=val0-ip1;
       err1=val1-ip2;
    }
    void Predict(int32_t val0,int32_t val1,int32_t &err0,int32_t &err1,int32_t &p0_a,int32_t &p0_b,int32_t &p1_a,int32_t &p1_b)
    {

    }
    void Unpredict(int32_t err0,int32_t err1,int32_t &val0,int32_t &val1)
    {
       /*int32_t p1a=lpa.Predict(valb);
       val0=err0+p1a;
       int32_t p1b=lpb.Predict(val0);
       val1=err1+p1b;

       vala=val0;valb=val1;*/
    }
    void Update() {
      lpa.Update(e[0]);
      lpb.Update(e[1]);
    }
  private:
    double e[4];
    LPC2 lpa,lpb;
    int n[4];
    vector <FrameCoder::FrameStats> &stats;
};


class StereoProfileNormal : public StereoProcess {
  public:
    StereoProfileNormal(const SacProfile &profile,vector <FrameCoder::FrameStats> &stats)
    :lpc(2,LPC2(profile.Get(0),16,8,32,profile.Get(1))),
     lms1(2,LMSADA2(profile.Get(2),profile.Get(3),256,128)),
     lms2(2,LMSADA2(profile.Get(4),profile.Get(4),32,16)),
     lms3(2,LMSADA2(profile.Get(5),profile.Get(5),8,4)),
     //blend(2,BlendRPROP(3,0.005,0.0005,0.01)),
     lmsmix(2,SSLMS(3,0.001)),
     //lmsmix(2,BlendLM(profile.Get(0),3,1)),
     pv(3),stats(stats)
    {
      for (int i=0;i<4;i++) val[0][i]=val[1][i]=0;
    };
    double Predict(int ch0,int ch1)
    {
       p[0]=lpc[ch0].Predict(val[ch1][0]);
       p[1]=lms1[ch0].Predict(val[ch1][1]);
       p[2]=lms2[ch0].Predict(val[ch1][2]);
       p[3]=lms3[ch0].Predict(val[ch1][3]);

       pv[0]=p[1];pv[1]=pv[0]+p[2];pv[2]=pv[1]+p[3];

       p0=p[0];
       p1=lmsmix[ch0].Predict(pv);
       return p0+p1;
    }
    void CalcError(int ch,int32_t v)
    {
       val[ch][0]=v;
       val[ch][1]=val[ch][0]-p[0];
       val[ch][2]=val[ch][1]-p[1];
       val[ch][3]=val[ch][2]-p[2];
    }
    void Predict(int32_t val0,int32_t val1,int32_t &err0,int32_t &err1)
    {
       err0=val0-round(Predict(0,1));
       CalcError(0,val0);
       err1=val1-round(Predict(1,0));
       CalcError(1,val1);
    }
    void Unpredict(int32_t err0,int32_t err1,int32_t &val0,int32_t &val1)
    {
       int p0=round(Predict(0,1));
       if (stats[0].enc_mapped) err0=stats[0].mymap.Unmap(p0,err0);
       val0=err0+p0;
       CalcError(0,val0);
       //cout << "(" << err0 << " " << val0 << ")";

       int p1=round(Predict(1,0));
       if (stats[1].enc_mapped) err1=stats[1].mymap.Unmap(p1,err1);
       val1=err1+p1;
       CalcError(1,val1);
    }
    void Update() {
      for (int ch=0;ch<2;ch++) {
        lpc[ch].Update(val[ch][0]);
        lms1[ch].Update(val[ch][1]);
        lms2[ch].Update(val[ch][2]);
        lms3[ch].Update(val[ch][3]);
        lmsmix[ch].Update(val[ch][1]);
      }
    }
  private:
    double val[2][4],p[4];
    vector <LPC2> lpc;
    vector <LMSADA2> lms1,lms2,lms3;
    vector <SSLMS> lmsmix;
    Vector pv;
    vector <FrameCoder::FrameStats> &stats;
    double p0,p1;
};


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
    if (mean) {
        minval-=mean;
        maxval-=mean;
        for (int i=0;i<numsamples;i++) src[i]-=mean;
    }
    framestats[ch].minval=minval;
    framestats[ch].maxval=maxval;
    framestats[ch].mean=mean;
    cout << "mean: " << mean << ", min: " << minval << " ,max: " << maxval << endl;
  }
}


void FrameCoder::PredictStereoFrame(const SacProfile &profile,int ch0,int ch1,int from,int numsamples)
{
  StereoProcess *myProfile=nullptr;
  if (profile.type==0) myProfile=new StereoProfileFast(profile.coefs[0].vdef,framestats);
  else if (profile.type==1) myProfile=new StereoProfileNormal(profile,framestats);
  //else if (profile.type==2) myProfile=new StereoProfileHigh(profile,framestats);
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
      if (unmap_b != dst1[i]) cout << unmap_a << " " << dst0[i] << endl;
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
  StereoProcess *myProfile=nullptr;
  if (profile.type==0) myProfile=new StereoProfileFast(profile.coefs[0].vdef,framestats);
  else if (profile.type==1) myProfile=new StereoProfileNormal(profile,framestats);
  //else if (profile.type==2) myProfile=new StereoProfileHigh(profile,framestats);

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
    MapEncoder me(rc,framestats[ch].mymap.usedl,framestats[ch].mymap.usedh);
    me.Decode();
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
      CostGolomb():mean_err(0.998){};
      double Calc(const int32_t *buf,int numsamples)
      {
        double nbits=0;
        if (numsamples) {
          for (int i=0;i<numsamples;i++) {
            int32_t val=MathUtils::S2U(buf[i]);
            int m=max(static_cast<int>(mean_err.Get()),1);
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

  for (int i=0;i<5;i++)
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
      cout << best_x << endl;
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
    cout << "ms\n";
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

  //SacProfile optprofile=baseprofile;
  //Optimize(optprofile);
  if (numchannels==2) {
    //InterChannel(0,1,numsamples);
    PredictStereoFrame(baseprofile,0,1,0,numsamples);
  } else for (int ch=0;ch<numchannels;ch++) PredictMonoFrame(ch,numsamples);
}

void FrameCoder::Unpredict()
{
  if (numchannels==2) UnpredictStereoFrame(baseprofile,0,1,numsamples);
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
  cout << "  " << samplesprocessed << "/" << totalsamples << ":" << setw(6) << miscUtils::ConvertFixed(r,1) << "%\r";
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
  cout << endl;
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
  cout << endl;
}
