#ifndef VLE_H
#define VLE_H

#include "range.h"
#include "../model/counter.h"
#include "../model/map.h"
#include "../model/mixer.h"

#define h1y(v,k) (((v)>>k)^(v))
#define h2y(v,k) (((v)*2654435761UL)>>(k))

class BitplaneCoder {
  const int cntref_upd_rate=400;
  const int cntsig_upd_rate=300;
  const int mix_upd_rate=800;
  const int cntsse_upd_rate=1000;
  const int mixsse_upd_rate=400;
  public:
    BitplaneCoder(RangeCoderSH &rc,int maxbpn,int numsamples)
    :rc(rc),lmixref(32,NMixLogistic(3)),lmixsig(32,NMixLogistic(2)),
    finalmix(2),
    msb(numsamples),maxbpn(maxbpn),numsamples(numsamples)
    {
      bctx=state=0;
      bpn=0;
    }
    void GetSigState(int i) { // get actual significance state
      sigst[0]=msb[i];
      sigst[1]=i>0?msb[i-1]:0;
      sigst[2]=i<numsamples-1?msb[i+1]:0;
      sigst[3]=i>1?msb[i-2]:0;
      sigst[4]=i<numsamples-2?msb[i+2]:0;
      sigst[5]=i>2?msb[i-3]:0;
      sigst[6]=i<numsamples-3?msb[i+3]:0;
      sigst[7]=i>3?msb[i-4]:0;
      sigst[8]=i<numsamples-4?msb[i+4]:0;
    }
    int PredictRef()
    {
      int mixctx=(state&7);

      int val=pabuf[sample];

      int lval=sample>0?pabuf[sample-1]:0;
      int lval2=sample>1?pabuf[sample-2]:0;
      int nval=sample<(numsamples-1)?pabuf[sample+1]:0;

      /*int ctx1=((val>>(bpn+1))&15);
      int ctx2=(lval>>bpn)&15;
      int ctx3=(nval>>(bpn+1))&15;*/

      int lctx=0;
      if ((lval>>bpn)>((val>>(bpn+1))<<1)) lctx+=(1<<0);
      if (nval>>(bpn+1)>(val>>(bpn+1))) lctx+=(1<<1);
      if ((lval2>>bpn)>((val>>(bpn+1))<<1)) lctx+=(1<<2);

      pc1=&bpn1[0];
      pc2=&bpn1[1+msb[sample]];
      pc3=&bpn1[1+32+lctx];
      plmix=&lmixref[mixctx];

      vector <int>vp={pc1->p1,pc2->p1,pc3->p1};
      int px=plmix->Predict(vp);
      return px;
    }
    void UpdateRef(int bit) {
      pc1->update(bit,cntref_upd_rate);
      pc2->update(bit,cntref_upd_rate);
      pc3->update(bit,cntref_upd_rate);
      //pc4->update(bit,cntref_upd_rate);
      plmix->Update(bit,mix_upd_rate);
      state=(state<<1)+0;
    }
    int PredictSig() {
      int mixctx=(state&7);
      int ctx1=0;
      if (sigst[1]) ctx1+=(1<<0);
      if (sigst[2]) ctx1+=(1<<1);
      if (sigst[3]) ctx1+=(1<<2);
      if (sigst[4]) ctx1+=(1<<3);
      if (sigst[5]) ctx1+=(1<<4);
      if (sigst[6]) ctx1+=(1<<5);
      if (sigst[7]) ctx1+=(1<<6);
      if (sigst[8]) ctx1+=(1<<7);

      pc1=&bpn2[0];
      pc2=&bpn2[1+ctx1];

      plmix=&lmixsig[mixctx];
      vector <int>vp={pc1->p1,pc2->p1};
      return plmix->Predict(vp);
    }
    void UpdateSig(int bit) {
      pc1->update(bit,cntsig_upd_rate);
      pc2->update(bit,cntsig_upd_rate);
      plmix->Update(bit,mix_upd_rate);
      state=(state<<1)+1;
      sighist=(sighist<<1)+bit;
    }
    int PredictSSE(int p1) {
      int ssectx=(bpn<<1)+((sigst[0]>0));
      psse=&sse[ssectx];
      vector <int>vp={psse->Predict(p1),p1};
      return finalmix.Predict(vp);
    }
    void UpdateSSE(int bit) {
      psse->Update(bit,cntsse_upd_rate);
      finalmix.Update(bit,mixsse_upd_rate);
    }
    void Encode(int32_t *abuf)
    {
      pabuf=abuf;
      for (bpn=maxbpn;bpn>=0;bpn--)
      {
        bctx=1;
        state=0;
        sighist=1;
        for (sample=0;sample<numsamples;sample++) {
          GetSigState(sample);
          int bit=(pabuf[sample]>>bpn)&1;
          if (sigst[0]) { // coef is significant, refine
            rc.EncodeBitOne(PredictSSE(PredictRef()),bit);
            UpdateRef(bit);
            UpdateSSE(bit);
          } else { // coef is insignificant
            rc.EncodeBitOne(PredictSSE(PredictSig()),bit);
            UpdateSig(bit);
            UpdateSSE(bit);
            if (bit) msb[sample]=bpn;
          }
          bctx=((bctx<<1)+bit)&15;
        }
      }
    }
    void Decode(int32_t *buf)
    {
      int bit;
      pabuf=buf;
      for (int i=0;i<numsamples;i++) buf[i]=0;
      for (bpn=maxbpn;bpn>=0;bpn--)
      {
        bctx=1;
        state=0;
        for (sample=0;sample<numsamples;sample++) {
          GetSigState(sample);
          if (sigst[0]) { // coef is significant, refine
            bit=rc.DecodeBitOne(PredictSSE(PredictRef()));
            UpdateRef(bit);
            UpdateSSE(bit);
            if (bit) buf[sample]+=(1<<bpn);
          } else { // coef is insignificant
            bit=rc.DecodeBitOne(PredictSSE(PredictSig()));
            UpdateSig(bit);
            UpdateSSE(bit);
            if (bit) {
              buf[sample]+=(1<<bpn);
              msb[sample]=bpn;
            }
          }
        }
      }
      for (int i=0;i<numsamples;i++) buf[i]=MathUtils::U2S(buf[i]);
    }
  private:
    RangeCoderSH &rc;
    LinearCounter16 bpn1[1<<10],bpn2[1<<10];
    vector <NMixLogistic>lmixref,lmixsig;
    NMixLogistic finalmix;
    SSENL<32> sse[64];
    SSENL<32> *psse;
    LinearCounter16 *pc1,*pc2,*pc3,*pc4,*pc5;
    NMixLogistic *plmix;
    int *pabuf,sample;
    vector <int>msb;
    int sigst[9];
    int maxbpn,bpn,numsamples;
    uint32_t bctx,state,sighist;
};

class Golomb {
  public:
    Golomb (RangeCoderSH &rc)
    :msum(0.8,1<<15),rc(rc)
    {
      lastl=0;
    }
    void Encode(int val)
    {
      if (val<0) val=2*(-val);
      else if (val>0) val=(2*val)-1;

      int m=max(static_cast<int>(msum.Get()),1);
      int q=val/m;
      int r=val-q*m;

      //for (int i=0;i<q;i++) rc.EncodeBitOne(PSCALEh,1); // encode exponent unary
      //rc.EncodeBitOne(PSCALEh,0);

      int ctx=1;
      for (int i=7;i>=0;i--) {
        int bit=(q>>i)&1;
        rc.EncodeBitOne(cnt[ctx].p1,bit);
        cnt[ctx].update(bit,250);

        ctx+=ctx+bit;
      }

      /*int ctx=0;
      for (int i=0;i<q;i++) {
        int pctx=lastl+(ctx<<1);
        rc.EncodeBitOne(cnt[pctx].p1,1);
        cnt[pctx].update(1,128);
        ctx++;
        if (ctx>1) ctx=1;
      }
      int pctx=lastl+(ctx<<1);
      rc.EncodeBitOne(cnt[pctx].p1,0);
      cnt[pctx].update(0,128);

      if (q>0) lastl=1;
      else lastl=0;*/

      if (m>1)
      {
        int b=ceil(log(m)/log(2));
        int t=(1<<b)-m;
        if (r < t) {
          for (int i=b-2;i>=0;i--) rc.EncodeBitOne(PSCALEh,((r>>i)&1));
        } else {
          for (int i=b-1;i>=0;i--) rc.EncodeBitOne(PSCALEh,(((r+t)>>i)&1));
        }
      }

      msum.Update(val);
    }
    int Decode() {
      int q=0;
      while (rc.DecodeBitOne(PSCALEh)!=0) q++;

      int m=max(static_cast<int>(msum.Get()),1);
      int r=0;

      if (m>1)
      {
        int b=ceil(log(m)/log(2));
        int t=(1<<b)-m;
        for (int i=b-2;i>=0;i--) r=(r<<1)+rc.DecodeBitOne(PSCALEh);
        if (r>=t) r=((r<<1)+rc.DecodeBitOne(PSCALEh))-t;
      }

      int val=m*q+r;
      msum.Update(val);

      if (val) {
        if (val&1) val=((val+1)>>1);
        else val=-(val>>1);
      }
      return val;
    }
    RunExp msum;
  private:
    RangeCoderSH &rc;
    LinearCounter16 cnt[512];
    int lastl;
};

class GolombRC {
  public:
    GolombRC (RangeCoder &rc)
    :msum(0.8,1<<15),rc(rc)
    {
    }
    void Encode(int val)
    {
      if (val<0) val=2*(-val);
      else if (val>0) val=(2*val)-1;

      int m=max(static_cast<int>(msum.Get()),1);
      int q=val/m;
      int r=val-q*m;

      for (int i=0;i<q;i++) rc.EncodeBitOne(PSCALEh,1); // encode exponent unary
      rc.EncodeBitOne(PSCALEh,0);

      rc.EncodeSymbol(r,1,m);

      msum.Update(val);
    }
    int Decode() {
      int q=0;
      while (rc.DecodeBitOne(PSCALEh)!=0) q++;

      int m=max(static_cast<int>(msum.Get()),1);

      int r=rc.DecProb(m);
      rc.DecodeSymbol(r,1);

      int val=m*q+r;
      msum.Update(val);

      if (val) {
        if (val&1) val=((val+1)>>1);
        else val=-(val>>1);
      }
      return val;
    }
    RunExp msum;
  private:
    RangeCoder &rc;
};


#endif
