#ifndef CODEC_H
#define CODEC_H

#include "file/wav.h"
#include "file/sac.h"
#include "pred/predictor.h"
#include "coder/range.h"
#include "coder/vle.h"

class SacProfile {
  public:
    struct coef {
      double vmin,vmax,vdef;
    };
      SacProfile():type(0){};
      void Init(int numcoefs,int ptype)
      {
         coefs.resize(numcoefs);
         type=ptype;
      }
      SacProfile(int numcoefs,int type)
      :type(type),coefs(numcoefs)
      {

      }
      void Set(int num,double vmin,double vmax,double vdef)
      {
        if (num>=0 && num< static_cast<int>(coefs.size())) {
          coefs[num].vmin=vmin;
          coefs[num].vmax=vmax;
          coefs[num].vdef=vdef;
        }
      }
      double Get(int num) const {
        if (num>=0 && num< static_cast<int>(coefs.size())) {
            return coefs[num].vdef;
        } else return 0.;
      }
      int type;
      vector <coef> coefs;
};

class CostFunction {
  public:
      CostFunction(){};
      virtual double Calc(const int32_t *buf,int numsamples)=0;
      virtual ~CostFunction(){};
};

class MapEncoder {
  const int cnt_upd_rate=500;
  const int mix_upd_rate=1000;
  public:
    MapEncoder(RangeCoderSH &rc,vector <bool>&usedl,vector <bool>&usedh)
    :rc(rc),mixl(3),mixh(3),ul(usedl),uh(usedh)
    {

    }
    int PredictLow(int i)
    {
       int ctx1=ul[i-1];
       int ctx2=uh[i-1];
       int ctx3=i>1?ul[i-2]:0;
       pc1=&cnt[ctx1];
       pc2=&cnt[1+ctx2];
       pc3=&cnt[2+ctx3];
       vector <int>p={pc1->p1,pc2->p1,pc3->p1};
       return mixl.Predict(p);
    }
    int PredictHigh(int i)
    {
       int ctx1=uh[i-1];
       int ctx2=ul[i];
       int ctx3=i>1?uh[i-2]:0;
       pc1=&cnt[4+ctx1];
       pc2=&cnt[4+1+ctx2];
       pc3=&cnt[4+2+ctx3];
       vector <int>p={pc1->p1,pc2->p1,pc3->p1};
       return mixh.Predict(p);
    }
    void UpdateLow(int bit)
    {
       pc1->update(bit,cnt_upd_rate);
       pc2->update(bit,cnt_upd_rate);
       pc3->update(bit,cnt_upd_rate);
       mixl.Update(bit,mix_upd_rate);
    }
    void UpdateHigh(int bit)
    {
       pc1->update(bit,cnt_upd_rate);
       pc2->update(bit,cnt_upd_rate);
       pc3->update(bit,cnt_upd_rate);
       mixh.Update(bit,mix_upd_rate);
    }
    void Encode()
    {
       int p1,bit;
       for (int i=1;i<=1<<15;i++) {
          p1=PredictLow(i);
          bit=ul[i];
          rc.EncodeBitOne(p1,bit);
          UpdateLow(bit);

          p1=PredictHigh(i);
          bit=uh[i];
          rc.EncodeBitOne(p1,bit);
          UpdateHigh(bit);
       }
    }
    void Decode()
    {
       int p1,bit;
       for (int i=1;i<=1<<15;i++) {
          p1=PredictLow(i);
          bit=rc.DecodeBitOne(p1);
          UpdateLow(bit);
          ul[i]=bit;

          p1=PredictHigh(i);
          bit=rc.DecodeBitOne(p1);
          UpdateHigh(bit);
          uh[i]=bit;
       }
    }
  private:
    RangeCoderSH &rc;
    LinearCounter16 cnt[8];
    LinearCounter16 *pc1,*pc2,*pc3;
    NMixLogistic mixl,mixh;
    vector <bool>&ul,&uh;
};

class Remap {
  public:
    Remap()
    :scale(1<<15),usedl(scale+1),usedh(scale+1)
    {
    }
    void Reset() {
      fill(begin(usedl),end(usedl),0);
      fill(begin(usedh),end(usedh),0);
      vmin=vmax=0;
    }
    void Analyse(int32_t *src,int numsamples)
    {
       for (int i=0;i<numsamples;i++) {
        int val=src[i];
        if (val>0) {
          if (val>scale) cout << "val too large: " << val << endl;
          else {
             if (val>vmax) vmax=val;
             usedh[val]=true;
          }
        } else if (val<0) {
          val=(-val);
          if (val>scale) cout << "val too large: " << val << endl;
          else {
            if (val>vmin) vmin=val;
            usedl[val]=true;
          }
        }
       }
       //for (int i=1;i<=scale;i++) usedl[i]=usedh[i]=true;
    }
    bool isUsed(int val) {
      if (val>scale) return false;
      if (val<-scale) return false;
      if (val>0) return usedh[val];
      if (val<0) return usedl[-val];
      return true;
    }
    int32_t Map(int32_t pred,int32_t err)
    {
       int sgn=1;
       if (err==0) return 0;
       if (err<0) {err=-err;sgn=-1;};

       int merr=0;
       for (int i=1;i<=err;i++) {
          if (isUsed(pred+(sgn*i))) merr++;
       }
       return sgn*merr;
    }
    int32_t Unmap(int32_t pred,int32_t merr)
    {
       int sgn=1;
       if (merr==0) return 0;
       if (merr<0) {merr=-merr;sgn=-1;};

       int err=1;
       int terr=0;
       while (1) {
         if (isUsed(pred+(sgn*err))) terr++;
         if (terr==merr) break;
         err++;
       }
       return sgn*err;
    }
  int scale,vmin,vmax;
  vector <bool>usedl,usedh;
};

class FrameCoder {
  public:
    struct FrameStats {
      int maxbpn,maxbpn_map;
      bool enc_mapped;
      int32_t minval,maxval,mean;
      Remap mymap;
    };
    FrameCoder(int numch,int framesz,int profile);
    void SetNumSamples(int nsamples){numsamples=nsamples;};
    int GetNumSamples(){return numsamples;};
    void Predict();
    void Unpredict();
    void Encode();
    void Decode();
    void WriteEncoded(AudioFile &fout);
    void ReadEncoded(AudioFile &fin);
    vector <vector<int32_t>>samples,err0,err1,error,s2u_error,s2u_error_map;
    vector <BufIO> encoded,enc_temp1,enc_temp2;
    vector <FrameStats> framestats;
  private:
    void InterChannel(int ch0,int ch1,int numsamples);
    int EncodeMonoFrame_Normal(int ch,int numsamples,BufIO &buf);
    int EncodeMonoFrame_Mapped(int ch,int numsamples,BufIO &buf);
    void Optimize(SacProfile &profile);
    double GetCost(SacProfile &profile,CostFunction *func,int coef,double testval,int start_sample,int samples_to_optimize);
    void PredictMonoFrame(int ch,int numsamples);
    void UnpredictMonoFrame(int ch,int numsamples);

    void AnalyseChannel(int ch,int numsamples);

    void PredictStereoFrame(const SacProfile &profile,int ch0,int ch1,int from,int numsamples);
    void UnpredictStereoFrame(const SacProfile &profile,int ch0,int ch1,int numsamples);

    void EncodeMonoFrame(int ch,int numsamples);
    void DecodeMonoFrame(int ch,int numsamples);
    int numchannels,framesize,numsamples;
    SacProfile baseprofile;
};

class Codec {
  public:
    Codec():framesize(0){};
    void EncodeFile(Wav &myWav,Sac &mySac,int profile);
    void DecodeFile(Sac &mySac,Wav &myWav);
  private:
    void PrintProgress(int samplesprocessed,int totalsamples);
    int framesize;
};

#endif
