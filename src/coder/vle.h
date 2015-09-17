#ifndef VLE_H
#define VLE_H

#include "range.h"
#include "counter.h"
#include "map.h"

class ExpSmoother
{
  public:
      ExpSmoother(double alpha):alpha(alpha){sum=0;};
      ExpSmoother(double alpha,double sum):alpha(alpha),sum(sum){};
      double Get(){return sum;};
      void Update(double val) {
        sum=alpha*sum+(1-alpha)*val;
      }
  private:
    double alpha,sum;
};

class BPNCoder {
  public:
    BPNCoder(RangeCoderSH &rc,int maxbpn):rc(rc),maxbpn(maxbpn)
    {
    }
    void Encode(int val)
    {
      val=MathUtils::S2U(val);
      int msb=0;
      for (int i=maxbpn;i>=0;i--) {
        int bit=(val>>i)&1;

        int bctx=i+(msb<<5);
        int ssectx=i+((msb>0)<<6);

        int p1=bpn[bctx].p1;
        int p1sse=sse[ssectx].p1(p1);

        rc.EncodeBitOne(p1sse,bit);
        bpn[bctx].update(bit,250);
        sse[ssectx].update2(bit,220);

        if (msb==0 && bit) msb=i;
     }
    }
     int Decode()
     {
      int val=0;
      int msb=0;
      for (int i=maxbpn;i>=0;i--) {
        int bctx=i+(msb<<5);
        int ssectx=i+((msb>0)<<6);

        int p1=bpn[bctx].p1;
        int p1sse=sse[ssectx].p1(p1);

        int bit=rc.DecodeBitOne(p1sse);
        bpn[bctx].update(bit,250);
        sse[ssectx].update2(bit,220);

        if (msb==0 && bit) msb=i;
        val+=(bit<<i);
      }
      return MathUtils::U2S(val);
     }
  private:
    RangeCoderSH &rc;
    LinearCounter16 bpn[1<<12];
    SSE<4> sse[256];
    int maxbpn;
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

      for (int i=0;i<q;i++) rc.EncodeBitOne(PSCALEh,1); // encode exponent unary
      rc.EncodeBitOne(PSCALEh,0);

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
    ExpSmoother msum;
  private:
    RangeCoderSH &rc;
    LinearCounter16 cnt[256];
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
    ExpSmoother msum;
  private:
    RangeCoder &rc;
};


#endif
