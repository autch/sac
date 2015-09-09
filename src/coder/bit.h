#ifndef BIT_H
#define BIT_H

#include "../global.h"

class BufIO {
  public:
      BufIO():buf(1024){Reset();};
      BufIO(int initsize):buf(initsize){Reset();};
      void Reset(){bufpos=0;};
      void PutByte(int val)
      {
        if (bufpos>=buf.size()) buf.resize(buf.size()*2);
        buf[bufpos++]=val;
      }
      int GetByte() {
        if (bufpos>=buf.size()) return -1;
        else return buf[bufpos++];
      }
      size_t GetBufPos(){return bufpos;};
      vector <uint8_t> &GetBuf(){return buf;};
  private:
     size_t bufpos;
     vector <uint8_t>buf;
};

// Binary RangeCoder with Carry and 64-bit low
// derived from rc_v3 by Eugene Shelwien

#define PBITS   (15)
#define PSCALE  (1<<PBITS)
#define PSCALEh (PSCALE>>1)
#define PSCALEm (PSCALE-1)

//#define SCALE_RANGE (((PSCALE-p1)*uint64_t(range)) >> PBITS) // 64 bit shift
#define SCALE_RANGE ((uint64_t(range)*((PSCALE-p1)<<(32-PBITS)))>>32)

class RangeCoderSH {
  enum { NUM=4,TOP=0x01000000U,Thres=0xFF000000U};
  public:
    RangeCoderSH(BufIO &buf,int dec=0):buf(buf),decode(dec){};
    void SetDecode(){decode=1;};
    void SetEncode(){decode=0;};
    void Init();
    void Stop();
    void EncodeBitOne(uint32_t p1,int bit);
    int  DecodeBitOne(uint32_t p1);
  protected:
    void ShiftLow();
    uint32_t range,code,FFNum,Cache;
    uint64_t lowc;
    BufIO &buf;
    int decode;
};
#endif // BIT_H
