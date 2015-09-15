#ifndef PREDICTOR_H
#define PREDICTOR_H

#include "../global.h"
#include "../math/cholesky.h"
#include "../math/cov.h"

// general purpose predictor class
class MonoPredictor {
  public:
      virtual int32_t Predict()=0;
      virtual void Update(int32_t val)=0;
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

class LPC {
  public:
      LPC(double alpha,int order,int k)
      :cov(alpha,order),sol(order),order(order),k(k),nk(0)
      {

      }
      int32_t Predict() {
        double sum=0.;
        for (int i=0;i<order;i++) sum+=sol[i]*cov.hist[i];
        return floor(sum+0.5);
      }
      void Update(int32_t val) {
        cov.UpdateCov();
        cov.UpdateB(val);
        cov.UpdateHist(val,0,order-1);
        nk++;
        if (nk>=k) {
          nk=0;
          if (chol.Factor(cov.c)) {
            cout << "matrix indefinit" << endl;
          } else chol.Solve(cov.b,sol);
        }
      }
  private:
    AutoCov cov;
    Cholesky chol;
    Vector sol;
    int order,k,nk;
};

class LPC2 {
  public:
      LPC2(double alpha,int S,int D,int k)
      :cov(alpha,S+D),sol(S+D),S(S),D(D),k(k),nk(0)
      {

      }
      int32_t Predict(int32_t val) {
        cov.UpdateHist(val,S,(S+D)-1);
        double sum=0.;
        for (int i=0;i<S+D;i++) sum+=sol[i]*cov.hist[i];
        //return static_cast<int>(sum+0.5);
        return floor(sum+0.5);
      }
      void Update(int32_t val0) {
        cov.UpdateCov();
        cov.UpdateB(val0);
        cov.UpdateHist(val0,0,S-1);

        nk++;
        if (nk>=k) {
          nk=0;
          if (chol.Factor(cov.c)) {
            cout << "matrix indefinit" << endl;
          } else chol.Solve(cov.b,sol);
        }
      }
  private:
    AutoCov cov;
    Cholesky chol;
    Vector sol;
    int S,D,k,nk;
};

class NLMS {
  public:
      NLMS(double mu,int order):w(order),hist(order),mu(mu),order(order) {
        pred=0;
        spow=0.0;
        imin=-(1<<15);imax=(1<<15);
      }
      int32_t Predict()
      {
        double sum=0.; // caluclate prediction
        for (int i=0;i<order;i++) sum+=w[i]*hist[i];
        pred=floor(sum+0.5);

        if(pred<imin)pred=imin;
        else if (pred>imax)pred=imax;

        return pred;
      }
      void Update(int32_t val) {
        //if (val<imin) imin=val;
        //else if (val>imax) imax=val;
        double err=val-pred; // calculate prediction error

        //spow-=(hist[order-1]*hist[order-1]); // update energy in the tapping window
        //spow+=(val*val);
        spow=0.992*spow+(val*val);

        const double wf=mu*err/(spow+0.001);
        for (int i=0;i<order;i++) w[i]+=wf*hist[i]; // update weight vector

        for (int i=order-1;i>0;i--) hist[i]=hist[i-1];
        hist[0]=val;
      }
  private:
    Vector w,hist;
    double spow,mu;
    int pred,order;
    int imin,imax;
};

#endif // PREDICTOR_H
