#ifndef COV_H
#define COV_H

#include "matrix.h"

// adaptively calculate auto-covariance-matrix

class AutoCov {
  public:
    AutoCov(double alpha,int dim):c(dim),hist(dim),b(dim),alpha(alpha)
    {
      for (int i=0;i<dim;i++) c[i][i]=1.0;
    };
    void UpdateCov() {
      const int n=hist.size();
      for (int j=0;j<n;j++) {
        double histj=hist[j];
        for (int i=0;i<n;i++) c[j][i]=alpha*c[j][i]+(1.0-alpha)*(hist[i]*histj);
      }
    }
    void UpdateB(double val)
    {
      const int n=hist.size();
      for (int i=0;i<n;i++) b[i]=alpha*b[i]+(1.0-alpha)*(val*hist[i]);

    }
    void UpdateHist(double val,int n0,int n1) {
      for (int i=n1;i>n0;i--) hist[i]=hist[i-1];
      hist[n0]=val;
    }
    Matrix c;
    Vector hist,b;
    double alpha;
};
#endif // COV_H
