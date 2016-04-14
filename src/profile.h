#ifndef PROFILE_H
#define PROFILE_H

#include <vector>
#include "pred/lpc.h"
#include "pred/nlms.h"
#include "pred/rls.h"
#include "coder/map.h"

class SacProfile {
  public:
    struct FrameStats {
      int maxbpn,maxbpn_map;
      bool enc_mapped;
      int32_t minval,maxval,mean;
      Remap mymap;
    };
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
      std::vector <coef> coefs;
};

// it's getting confusing
// predict two samples at once
class StereoPredictor {
  public:
    virtual void Predict(int32_t val0,int32_t val1,int32_t &err0,int32_t &err1)=0;
    virtual void Unpredict(int32_t err0,int32_t err1,int32_t &val0,int32_t &val1)=0;
    virtual void Update()=0;
    virtual ~StereoPredictor(){};
  protected:
    int32_t t0,t1;
};

class StereoFast : public StereoPredictor {
  public:
    StereoFast(double alpha,std::vector <SacProfile::FrameStats> &stats);
    double Predict();
    void Update(int32_t val);
    void Predict(int32_t val0,int32_t val1,int32_t &err0,int32_t &err1);
    void Unpredict(int32_t err0,int32_t err1,int32_t &val0,int32_t &val1);
    void Update() {
    }
  private:
    double x[2];
    int ch0,ch1;
    std::vector <LPC2> lpc;
    std::vector <SacProfile::FrameStats> &stats;
};

class StereoNormal : public StereoPredictor {
  public:
    StereoNormal(const SacProfile &profile,std::vector <SacProfile::FrameStats> &stats);
    double Predict(int ch0,int ch1);
    void CalcError(int ch,int32_t v);
    void Predict(int32_t val0,int32_t val1,int32_t &err0,int32_t &err1);
    void Unpredict(int32_t err0,int32_t err1,int32_t &val0,int32_t &val1);
    void Update();
  private:
    double val[2][4],p[4];
    std::vector <LPC3> lpc;
    std::vector <LMSADA2> lms1,lms2,lms3;
    std::vector <RLS> lmsmix;
    std::vector<double> pv;
    std::vector <SacProfile::FrameStats> &stats;
    double p0,p1;
};

class StereoHigh : public StereoPredictor {
  public:
    StereoHigh(const SacProfile &profile,std::vector <SacProfile::FrameStats> &stats);
    double Predict(int ch0,int ch1);
    void CalcError(int ch,int32_t v);
    void Predict(int32_t val0,int32_t val1,int32_t &err0,int32_t &err1);
    void Unpredict(int32_t err0,int32_t err1,int32_t &val0,int32_t &val1);
    void Update();
  private:
    double val[2][5],p[5];
    std::vector <LPC3> lpc;
    std::vector <LMSADA2>lms1,lms2,lms3,lms4;
    std::vector <RLS> lmsmix;
    std::vector<double> pv;
    std::vector <SacProfile::FrameStats> &stats;
};


#endif // PROFILE_H
