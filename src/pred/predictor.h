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

class LPC : public MonoPredictor {
  public:
      LPC(double alpha,int order,int k)
      :cov(alpha,order),sol(order),order(order),k(k),nk(0)
      {

      }
      int32_t Predict() {
        double sum=0.;
        for (int i=0;i<order;i++) sum+=sol[i]*cov.hist[i];
        //return static_cast<int>(sum+0.5);
        return floor(sum+0.5);
      }
      void Update(int32_t val) {
        cov.Update(val);
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

#endif // PREDICTOR_H
