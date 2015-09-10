#ifndef RANGE_H
#define RANGE_H

#include "bufio.h"

// Binary RangeCoder with Carry and 64-bit low
// derived from rc_v3 by Eugene Shelwien

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

#endif // RANGE_H
