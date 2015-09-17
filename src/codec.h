#ifndef CODEC_H
#define CODEC_H

#include "file/wav.h"
#include "file/sac.h"
#include "pred/predictor.h"
#include "coder/range.h"
#include "coder/vle.h"

class FrameCoder {
  struct FrameStats {
    int maxbpn;
  };
  public:
    void Init(int numch,int framesz);
    void SetNumSamples(int nsamples){numsamples=nsamples;};
    int GetNumSamples(){return numsamples;};
    void Predict(int profile);
    void Unpredict(int profile);
    void Encode();
    void Decode();
    void WriteEncoded(AudioFile &fout);
    void ReadEncoded(AudioFile &fin);
    vector <vector<int32_t>>samples,pred,error,temp;
    vector <BufIO> encoded;
    vector <FrameStats> framestats;
  private:
    void PredictMonoFrame(int ch,int numsamples);
    void UnpredictMonoFrame(int ch,int numsamples);

    void PredictStereoFrame(int profile,int ch0,int ch1,int numsamples);
    void UnpredictStereoFrame(int profile,int ch0,int ch1,int numsamples);

    void EncodeMonoFrame(int ch,int numsamples);
    void DecodeMonoFrame(int ch,int numsamples);
    int numchannels,framesize,numsamples;
};

class Codec {
  public:
    Codec():framesize(0){};
    void EncodeFile(Wav &myWav,Sac &mySac,int profile);
    void DecodeFile(Sac &mySac,Wav &myWav);
  private:
    void PrintProgress(int samplesprocessed,int totalsamples);
    FrameCoder myFrame;
    int framesize;
};

#endif
