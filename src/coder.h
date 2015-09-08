#ifndef CODER_H
#define CODER_H

#include "wav.h"

class Coder {
  public:
    Coder():framesize(0){};
    void EncodeFile(Wav &myWav);
  private:
    int framesize;
    vector<vector<int32_t>>samplesdata;
};

#endif // CODER_H
