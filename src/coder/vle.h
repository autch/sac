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
      if (val<0) val=2*(-val);
      else if (val>0) val=(2*val)-1;

      msb=0;
      int ctx=1;

      for (int i=maxbpn;i>=0;i--) {
        int bit=(val>>i)&1;

        int c=i+(msb<<5);
        int p1=bpn[c].p1;
        rc.EncodeBitOne(p1,bit);
        bpn[c].update(bit,0.01*PSCALE);

        if (msb==0 && bit) msb=i;
        ctx+=ctx+bit;
     }
    }
  private:
    RangeCoderSH &rc;
    LinearCounter16 bpn[1<<12];
    int maxbpn,msb;
};

class BitCoder {
  public:
    BitCoder(RangeCoderSH &rc,int maxbpn):rc(rc),maxbpn(maxbpn)
    {
    }
    void Encode(int pred,int val)
    {
      int signp=val<0?1:0;
      int signs=pred<0?1:0;
      int smatch=(signp==signs)?1:0;

      rc.EncodeBitOne(sgn.p1,smatch);
      sgn.update(smatch,0.01*PSCALE);

      failedat=0;
      for (int i=maxbpn;i>=0;i--)
      {
        int bitp=(pred>>i)&1;
        int bits=(val>>i)&1;

        int bmatch=(bitp==bits)?1:0;

        int ctx=(failedat<<6)+(smatch << 5)+i;

        int p1=bpn[ctx].p1;
        rc.EncodeBitOne(p1,bmatch);

        bpn[ctx].update(bmatch,0.01*PSCALE);
        if (!bmatch) {failedat=i;}
      }
    }
  private:
    RangeCoderSH &rc;
    LinearCounter16 bpn[1<<12],spn[1<<10],sgn;
    int histb;
    int maxbpn;
    int failedat;
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
