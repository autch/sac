#ifndef GLOBAL_H
#define GLOBAL_H

#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>

using namespace std;

#define DO(n) for (uint32_t _=0;_<n;_++)

#define PBITS   (15)
#define PSCALE  (1<<PBITS)
#define PSCALEh (PSCALE>>1)
#define PSCALEm (PSCALE-1)

template <class T> T abs(T a){return ((a) < 0)?-(a):(a);}
template <class T> T clamp(T val,T min,T max) {return val<min?min:val>max?max:val;};
template <class T> T div_signed(T val,T s){return val<0?-(((-val)+(1<<(s-1)))>>s):(val+(1<<(s-1)))>>s;};

#endif // GLOBAL_H
