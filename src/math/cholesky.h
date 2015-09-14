#ifndef CHOLESKY_H
#define CHOLESKY_H

#include "../global.h"
#include "../utils.h"
#include "matrix.h"

// calculate the cholesky decomposition of a symmetric pos. def. matrix
class Cholesky {
  public:
    Cholesky(){};
    bool Test();
    int Factor(const Matrix &src);
    void Solve(const Vector &b,Vector &x);
    Matrix m;
  protected:
    Vector y;
};
#endif // CHOLESKY_H
