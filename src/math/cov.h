#ifndef COV_H
#define COV_H

#include "matrix.h"

// adaptively calculate auto-covariance-matrix

class AutoCov {
  public:
    AutoCov(double alpha,int dim):c(dim),r(dim),hist(dim),b(dim),alpha(alpha)
    {
      for (int i=0;i<dim;i++) c[i][i]=1.0;
    };
    void Update(double val) {
      int n=hist.size();

      // calculate autocorrelation matrix
      for (int j=0;j<n;j++) {
        for (int i=0;i<n;i++) r[j][i]=hist[i]*hist[j];
      }

      // update auto-covariance matrix
      for (int j=0;j<n;j++)
        for (int i=0;i<n;i++) {
            c[j][i]=alpha*c[j][i]+(1.0-alpha)*(r[j][i]);
        }

      // update b vector
      for (int i=0;i<n;i++) b[i]=alpha*b[i]+(1.0-alpha)*(val*hist[i]);

      for (int i=n-1;i>0;i--) hist[i]=hist[i-1];
      hist[0]=val;
    }
    Matrix c,r;
    Vector hist,b;
    double alpha;
};
#endif // COV_H
