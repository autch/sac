#ifndef UTILS_H
#define UTILS_H

#include "global.h"

// running exponential smoothing
// sum=alpha*sum+(1.0-alpha)*val, where 1/(1-alpha) is the mean number of samples considered
class RunExp {
  public:
      RunExp(double alpha):alpha(alpha){sum=0;};
      RunExp(double alpha,double sum):alpha(alpha),sum(sum){};
      inline double Get(){return sum;};
      inline void Update(double val) {
        sum=alpha*sum+(1.-alpha)*val;
      }
  private:
    double alpha,sum;
};

// running weighted sum: sum_{i=0}^n alpha^(n-i) val
class RunWeight {
  public:
      RunWeight(double alpha):alpha(alpha){sum=0;};
      inline double Get(){return sum;};
      inline void Update(double val) {
        sum=alpha*sum+val;
      }
  private:
    double alpha,sum;
};

class strUtils {
  public:
      static void strUpper(string &str)
      {
        std::transform(str.begin(), str.end(),str.begin(), ::toupper);
      }
};

class MathUtils {
  public:
      static int iLog2(int val) {
        int nbits=0;
        while (val>>=1) nbits++;
        return nbits;
      }
      static double SumDiff(const Vector &v1,const Vector &v2)
      {
         if (v1.size()!=v2.size()) return -1;
         else {
           double sum=0.;
           for (size_t i=0;i<v1.size();i++) sum+=fabs(v1[i]-v2[i]);
           return sum;
         }
      }
      static int32_t S2U(int32_t val)
      {
        if (val<0) val=2*(-val);
        else if (val>0) val=(2*val)-1;
        return val;
      }
      static int32_t U2S(int32_t val)
      {
        if (val&1) val=((val+1)>>1);
        else val=-(val>>1);
        return val;
      }
      static double L2Dist(const vector<double> &vec1,const vector<double> &vec2)
      {
         if (vec1.size()!=vec2.size()) return -1;
         else {
           double sum=0.;
           for (size_t i=0;i<vec1.size();i++) {double t=vec1[i]-vec2[i];sum+=t*t;};
           return sqrt(sum);
         }
      }
      static double L2Dist2(const vector<double> &vec1,const vector<double> &vec2)
      {
         if (vec1.size()!=vec2.size()) return -1;
         else {
           double sum=0.;
           for (size_t i=0;i<vec1.size();i++) {double t=vec1[i]-vec2[i];sum+=t*t;};
           return sum;
         }
      }
};

class miscUtils {
  public:
  // retrieve time string
  static string getTimeStrFromSamples(int numsamples,int samplerate)
  {
   ostringstream ss;
   int h,m,s,ms;
   h=m=s=ms=0;
   if (numsamples>0 && samplerate>0) {
     while (numsamples >= 3600*samplerate) {++h;numsamples-=3600*samplerate;};
     while (numsamples >= 60*samplerate) {++m;numsamples-=60*samplerate;};
     while (numsamples >= samplerate) {++s;numsamples-=samplerate;};
     ms=round((numsamples*1000.)/samplerate);
   }
   ss << setfill('0') << setw(2) << h << ":" << setw(2) << m << ":" << setw(2) << s << "." << ms;
   return ss.str();
 }
 static string getTimeStrFromSeconds(int seconds)
 {
   ostringstream ss;
   int h,m,s;
   h=m=s=0;
   if (seconds>0) {
      while (seconds >= 3600) {++h;seconds-=3600;};
      while (seconds >= 60) {++m;seconds-=60;};
      s=seconds;
   }
   ss << setfill('0') << setw(2) << h << ":" << setw(2) << m << ":" << setw(2) << s;
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
