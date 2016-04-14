#ifndef CODEC_H
#define CODEC_H

#include "file/wav.h"
#include "file/sac.h"
#include "coder/vle.h"
#include "coder/map.h"
#include "profile.h"

class CostFunction {
  public:
      CostFunction(){};
      virtual double Calc(const int32_t *buf,int numsamples)=0;
      virtual ~CostFunction(){};
};


class FrameCoder {
  public:
    FrameCoder(int numch,int framesz,int profile);
    void SetNumSamples(int nsamples){numsamples=nsamples;};
    int GetNumSamples(){return numsamples;};
    void Predict();
    void Unpredict();
    void Encode();
    void Decode();
    void WriteEncoded(AudioFile &fout);
    void ReadEncoded(AudioFile &fin);
    std::vector <std::vector<int32_t>>samples,err0,err1,error,s2u_error,s2u_error_map;
    std::vector <BufIO> encoded,enc_temp1,enc_temp2;
    std::vector <SacProfile::FrameStats> framestats;
  private:
    void InterChannel(int ch0,int ch1,int numsamples);
    int EncodeMonoFrame_Normal(int ch,int numsamples,BufIO &buf);
    int EncodeMonoFrame_Mapped(int ch,int numsamples,BufIO &buf);
    void Optimize(SacProfile &profile);
    double GetCost(SacProfile &profile,CostFunction *func,int coef,double testval,int start_sample,int samples_to_optimize);
    //void PredictMonoFrame(int ch,int numsamples);
    //void UnpredictMonoFrame(int ch,int numsamples);

    void AnalyseChannel(int ch,int numsamples);

    void PredictStereoFrame(const SacProfile &profile,int ch0,int ch1,int from,int numsamples);
    void UnpredictStereoFrame(const SacProfile &profile,int ch0,int ch1,int numsamples);

    void EncodeMonoFrame(int ch,int numsamples);
    void DecodeMonoFrame(int ch,int numsamples);
    int numchannels,framesize,numsamples;
    SacProfile baseprofile;
};

class Codec {
  public:
    Codec():framesize(0){};
    void EncodeFile(Wav &myWav,Sac &mySac,int profile);
    void DecodeFile(Sac &mySac,Wav &myWav);
  private:
    void PrintProgress(int samplesprocessed,int totalsamples);
    int framesize;
};

#endif
