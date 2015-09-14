#ifndef MATRIX_H
#define MATRIX_H

#include "../global.h"

// simple Square-Matrix class
class Matrix
{
  public:
    Matrix(){};
    Matrix(int dim):mat(dim,Vector(dim,0.))
    {
    }
    Vector& operator[] (size_t i) { return mat[i]; } // index operator

    // gets auto generated

    /*Matrix(const Matrix &m) // copy constructor
    :mat(m.mat)
    {
    }*/
    /*Matrix& operator= (const Matrix& m)  // copy assignment operator
    {
      if (this!=&m) mat=m.mat;
      return *this;
    }*/
    size_t GetDim()const {return mat.size();};
    vector <Vector> mat;
  private:
};

#endif // MATRIX_H
