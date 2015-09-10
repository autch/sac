#ifndef RICE_H
#define RICE_H

#include "range.h"
#include "counter.h"

class Rice {
  public:
    Rice (RangeCoderSH &rc):rc(rc)
    {
      K=12;
    }
    void Encode(int val)
    {
      if (val<0) val=2*(-val);
      else if (val>0) val=(2*val)-1;

      int Q=val>>K;
      for (int i=0;i<Q;i++) rc.EncodeBitOne(PSCALEh,1); // encode exponent unary
      rc.EncodeBitOne(PSCALEh,0);

      for (int i=K-1;i>=0;i--) {rc.EncodeBitOne(PSCALEh,(val>>i)&1);}; // encode remainder bits
    }
    int Decode() {
      int val=0;
      while (rc.DecodeBitOne(PSCALEh)!=0) val++;
      val<<=K;
      for (int i=K-1;i>=0;i--) {val+=(rc.DecodeBitOne(PSCALEh)<<i);};

      if (val&1) val=((val+1)>>1);
      else val=-(val>>1);
      return val;
    }
  private:
    RangeCoderSH &rc;
    int K;
};
#endif // RICE_H
