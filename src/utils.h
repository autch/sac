#ifndef UTILS_H
#define UTILS_H

#include "global.h"

class strUtils {
  public:
      static void strUpper(string &str)
      {
        std::transform(str.begin(), str.end(),str.begin(), ::toupper);
      }
};

class miscUtils {
  public:
      static void CpyVecCh(vector <uint8_t> &dest,const uint8_t* src,int len)
      {
        for (int i=0;i<len;i++) dest[i]=src[i];
      }
  // retrieve time string
  static string getTimeFromSamples(int numsamples,int samplerate)
  {
   ostringstream ss;
   int h,m,s,ms;
   h=m=s=ms=0;
   while (numsamples >= 3600*samplerate) {++h;numsamples-=3600*samplerate;};
   while (numsamples >= 60*samplerate) {++m;numsamples-=60*samplerate;};
   while (numsamples >= samplerate) {++s;numsamples-=samplerate;};
   ms=round((numsamples*1000.)/samplerate);
   ss << setfill('0') << setw(2) << h << ":" << setw(2) << m << ":" << setw(2) << s << "." << ms;
   return ss.str();
 }
 static string ConvertFixed(double val,int digits)
 {
   ostringstream ss;
   ss << fixed << setprecision(digits) << val;
   return ss.str();
 }
};

class binUtils {
  public:
      static uint32_t get32HL(const uint8_t *buf)
      {
        return((uint32_t)buf[3] + ((uint32_t)buf[2] << 8) +((uint32_t)buf[1] << 16) + ((uint32_t)buf[0] << 24));
      }
      static uint16_t get16LH(const uint8_t *buf)
      {
        return((uint16_t)buf[0] + ((uint16_t)buf[1] << 8));
      }
      static uint32_t get32LH(const uint8_t *buf)
      {
        return((uint32_t)buf[0] + ((uint32_t)buf[1] << 8) +((uint32_t)buf[2] << 16) + ((uint32_t)buf[3] << 24));
      }
      static void put16LH(uint8_t *buf,uint16_t val)
      {
        buf[0] = val & 0xff;
        buf[1] = (val>>8) & 0xff;
      }
      static void put32LH(uint8_t *buf,uint32_t val)
      {
        buf[0] = val & 0xff;
        buf[1] = (val>>8) & 0xff;
        buf[2] = (val>>16) & 0xff;
        buf[3] = (val>>24) & 0xff;
      }
      static int cnvU2S(int val,int bps)
      {
        return val>((1<<(bps-1))-1)?val-(1<<bps):val;
      }
      static string U322Str(uint32_t val)
      {
        string s;
        for (int i=0;i<4;i++) {s+=(char)(val & 0xff);val>>=8;};
        return s;
      }
};
#endif // UTILS_H
