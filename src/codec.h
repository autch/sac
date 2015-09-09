#ifndef CODER_H
#define CODER_H

#include "file/wav.h"
#include "file/sac.h"
#include "pred/predictor.h"
#include "coder/bit.h"

class Model {
  public:
    Model():framesize(0){};
    void EncodeFile(Wav &myWav,Sac &mySac);
    void PredictMonoFrame(int ch,int numsamples);
    void EncodeMonoFrame(int ch,int numsamples);
  private:
    int framesize;
    vector<vector<int32_t>>samplesdata;
    vector<BufIO>samplesenc;
};

#endif // CODER_H