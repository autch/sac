#include "profile.h"

StereoFast::StereoFast(double alpha,std::vector <SacProfile::FrameStats> &stats)
:lpc(2,LPC2(alpha,16,8,32)),stats(stats)
{
  x[0]=x[1]=0.;
  ch0=0;ch1=1;
};

double StereoFast::Predict()
{
  return lpc[ch0].Predict(x[ch1]);
}

void StereoFast::Update(int32_t val)
{
  lpc[ch0].Update(val);
  x[ch0]=val;
  std::swap(ch0,ch1);
}

void StereoFast::Predict(int32_t val0,int32_t val1,int32_t &err0,int32_t &err1)
{
  double pa=Predict();
  Update(val0);

  double pb=Predict();
  Update(val1);

  err0=val0-round(pa);
  err1=val1-round(pb);
}

void StereoFast::Unpredict(int32_t err0,int32_t err1,int32_t &val0,int32_t &val1)
{
  double pa=Predict();
  val0=err0+round(pa);
  Update(val0);

  double pb=Predict();
  val1=err1+round(pb);
  Update(val1);
}


StereoNormal::StereoNormal(const SacProfile &profile,std::vector <SacProfile::FrameStats> &stats)
:lpc(2,LPC3(profile.Get(0),16,8,8,500)),
 lms1(2,LMSADA2(256,128,profile.Get(1),profile.Get(2))),
 lms2(2,LMSADA2(32,16,profile.Get(3),profile.Get(3))),
 lms3(2,LMSADA2(8,4,profile.Get(4),profile.Get(4))),
 lmsmix(2,RLS(3,profile.Get(5))),
 pv(3),stats(stats)
{
  for (int i=0;i<4;i++) val[0][i]=val[1][i]=0;
};

double StereoNormal::Predict(int ch0,int ch1)
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

void StereoNormal::CalcError(int ch,int32_t v)
{
  val[ch][0]=v;
  val[ch][1]=val[ch][0]-p[0];
  val[ch][2]=val[ch][1]-p[1];
  val[ch][3]=val[ch][2]-p[2];
}

void StereoNormal::Predict(int32_t val0,int32_t val1,int32_t &err0,int32_t &err1)
{
  int32_t p0=(int32_t)round(Predict(0,1));
  err0=val0-p0;
  CalcError(0,val0);

  int32_t p1=(int32_t)round(Predict(1,0));
  err1=val1-p1;
  CalcError(1,val1);
}

void StereoNormal::Unpredict(int32_t err0,int32_t err1,int32_t &val0,int32_t &val1)
{
  int p0=round(Predict(0,1));
  if (stats[0].enc_mapped) err0=stats[0].mymap.Unmap(p0,err0);
  val0=err0+p0;
  CalcError(0,val0);

  int p1=round(Predict(1,0));
  if (stats[1].enc_mapped) err1=stats[1].mymap.Unmap(p1,err1);
  val1=err1+p1;
  CalcError(1,val1);
}

void StereoNormal::Update()
{
  for (int ch=0;ch<2;ch++) {
    lpc[ch].Update(val[ch][0]);
    lms1[ch].Update(val[ch][1]);
    lms2[ch].Update(val[ch][2]);
    lms3[ch].Update(val[ch][3]);
    lmsmix[ch].Update(val[ch][1]);
  }
}

StereoHigh::StereoHigh(const SacProfile &profile,std::vector <SacProfile::FrameStats> &stats)
:lpc(2,LPC3(profile.Get(0),16,8,8,500)),
 lms1(2,LMSADA2(1280,640,profile.Get(1),profile.Get(2))),
 lms2(2,LMSADA2(256,128,profile.Get(3),profile.Get(4))),
 lms3(2,LMSADA2(32,16,profile.Get(5),profile.Get(5))),
 lms4(2,LMSADA2(8,4,profile.Get(6),profile.Get(6))),
 lmsmix(2,RLS(4,profile.Get(7))),
     //lmsmix(2,BlendRPROP(4,0.005,0.001,0.01)),
 pv(4),stats(stats)
{
  for (int i=0;i<5;i++) val[0][i]=val[1][i]=0;
};

double StereoHigh::Predict(int ch0,int ch1)
{
  p[0]=lpc[ch0].Predict(val[ch1][0]);
  p[1]=lms1[ch0].Predict(val[ch1][1]);
  p[2]=lms2[ch0].Predict(val[ch1][2]);
  p[3]=lms3[ch0].Predict(val[ch1][3]);
  p[4]=lms4[ch0].Predict(val[ch1][4]);

  pv[0]=p[1];pv[1]=pv[0]+p[2];pv[2]=pv[1]+p[3];pv[3]=pv[2]+p[4];

  double p0=p[0];
  double p1=lmsmix[ch0].Predict(pv);
  return p0+p1;
}

void StereoHigh::CalcError(int ch,int32_t v)
{
  val[ch][0]=v;
  val[ch][1]=val[ch][0]-p[0];
  val[ch][2]=val[ch][1]-p[1];
  val[ch][3]=val[ch][2]-p[2];
  val[ch][4]=val[ch][3]-p[3];
}

void StereoHigh::Predict(int32_t val0,int32_t val1,int32_t &err0,int32_t &err1)
{
  int32_t p0=(int32_t)round(Predict(0,1));
  err0=val0-p0;
  CalcError(0,val0);

  int32_t p1=(int32_t)round(Predict(1,0));
  err1=val1-p1;
  CalcError(1,val1);
}

void StereoHigh::Unpredict(int32_t err0,int32_t err1,int32_t &val0,int32_t &val1)
{
  int p0=clamp((int32_t)round(Predict(0,1)),stats[0].minval,stats[0].maxval);

  if (stats[0].enc_mapped) err0=stats[0].mymap.Unmap(p0,err0);
  val0=err0+p0;
  CalcError(0,val0);
       //cout << "(" << err0 << " " << val0 << ")";

  int p1=clamp((int32_t)round(Predict(1,0)),stats[1].minval,stats[1].maxval);
  if (stats[1].enc_mapped) err1=stats[1].mymap.Unmap(p1,err1);
  val1=err1+p1;
  CalcError(1,val1);
}

void StereoHigh::Update()
{
  for (int ch=0;ch<2;ch++) {
    lpc[ch].Update(val[ch][0]);
    lms1[ch].Update(val[ch][1]);
    lms2[ch].Update(val[ch][2]);
    lms3[ch].Update(val[ch][3]);
    lms4[ch].Update(val[ch][4]);
    lmsmix[ch].Update(val[ch][1]);
  }
}
