#ifndef PREDICTOR_H
#define PREDICTOR_H

#include "../global.h"

// general purpose mono-predictor class
class MonoPredictor {
  public:
      virtual int32_t Predict()=0;
      virtual void Update(int32_t val)=0;
      virtual ~MonoPredictor(){};
};

// general purpose stereo-predictor class
class StereoPredictor {
  public:
      virtual int32_t Predict(int32_t val1)=0;
      virtual void Update(int32_t val0)=0;
      virtual ~StereoPredictor(){};
};

// it's getting confusing
// predict two samples at once
class StereoProcess {
  public:
    virtual void Predict(int32_t val0,int32_t val1,int32_t &err0,int32_t &err1)=0;
    virtual void Unpredict(int32_t err0,int32_t err1,int32_t &val0,int32_t &val1)=0;
    virtual void Update()=0;
    virtual ~StereoProcess(){};
  protected:
    int32_t t0,t1;
};

class StereoFromMono : public StereoProcess
{
  public:
    StereoFromMono(MonoPredictor *p0,MonoPredictor *p1)
    :p0(p0),p1(p1)
    {
    }
    void Predict(int32_t val0,int32_t val1,int32_t &err0,int32_t &err1)
    {
      t0=val0;t1=val1;
      err0=t0-p0->Predict();
      err1=t1-p1->Predict();
    }
    void Unpredict(int32_t err0,int32_t err1,int32_t &val0,int32_t &val1)
    {
      val0=t0=err0+p0->Predict();
      val1=t1=err1+p1->Predict();
    }
    void Update() {p0->Update(t0);p1->Update(t1);};
    ~StereoFromMono(){delete p0;delete p1;};
  protected:
    MonoPredictor *p0,*p1;
};

class StereoFromStereo : public StereoProcess {
  public:
    StereoFromStereo(StereoPredictor *p0,StereoPredictor *p1)
    :p0(p0),p1(p1),last(0)
    {
    }
    void Predict(int32_t val0,int32_t val1,int32_t &err0,int32_t &err1)
    {
      t0=val0;t1=val1;
      err0=t0-p0->Predict(last);
      err1=t1-p1->Predict(t0);
    }
    void Unpredict(int32_t err0,int32_t err1,int32_t &val0,int32_t &val1)
    {
      val0=t0=err0+p0->Predict(last);
      val1=t1=err1+p1->Predict(t0);
    }
    void Update() {p0->Update(t0);p1->Update(t1);last=t1;};
    ~StereoFromStereo(){delete p0;delete p1;};
  private:
    StereoPredictor *p0,*p1;
    int last;
};

class O1Pred : public MonoPredictor {
  public:
      O1Pred():mul(0),shift(0),last(0){};
      O1Pred(int mul,int shift):mul(mul),shift(shift),last(0){};
      int32_t Predict() {
        return (last*mul)>>shift;
      }
      void Update(int32_t val) {
        last=val;
      }
  private:
    int32_t mul,shift,last;
};



#endif // PREDICTOR_H
