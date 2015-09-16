#ifndef LPC_H
#define LPC_H

#include "../math/cholesky.h"
#include "../math/cov.h"

class LPC : public MonoPredictor {
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

class LPC2 : public StereoPredictor {
  public:
      LPC2(double alpha,int S,int D,int k)
      :cov(alpha,S+D),sol(S+D),S(S),D(D),k(k),nk(0)
      {

      }
      int32_t Predict(int32_t val1) {
        cov.UpdateHist(val1,S,(S+D)-1);
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
      //~LPC2(){cout << "lpc2 destruct\n";};
  private:
    AutoCov cov;
    Cholesky chol;
    Vector sol;
    int S,D,k,nk;
};

#endif // LPC_H
