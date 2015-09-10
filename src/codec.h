#ifndef CODER_H
#define CODER_H

#include "file/wav.h"
#include "file/sac.h"
#include "pred/predictor.h"
#include "coder/range.h"
#include "coder/rice.h"

class Codec {
  public:
    Codec():framesize(0){};
    void EncodeFile(Wav &myWav,Sac &mySac);
    void DecodeFile(Sac &mySac,Wav &myWav);
    void PredictMonoFrame(int ch,int numsamples);
    void EncodeMonoFrame(int ch,int numsamples);
    void DecodeMonoFrame(int ch,int numsamples);
  private:
    int framesize;
    vector<vector<int32_t>>samplesdata;
    vector<vector<int32_t>>samplestemp;
    vector<BufIO>samplesenc;
};

#endif // CODER_H
