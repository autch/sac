#include "codec.h"

void Model::PredictMonoFrame(int ch,int numsamples)
{
  O1Pred myPred;
  int32_t *data=&(samplesdata[ch][0]);

  for (int i=0;i<numsamples;i++) {
    int32_t val=data[i];
    int32_t pred=myPred.Predict();

    int32_t err=val-pred;

    data[i]=err;
    myPred.Update(val);
  }
}

void Model::EncodeMonoFrame(int ch,int numsamples)
{
  const int32_t *data=&(samplesdata[ch][0]);

  RangeCoderSH Coder(samplesenc[ch]);
  Coder.Init();

  int K=5;
  for (int i=0;i<numsamples;i++) {
    int val=data[i];
    if (val<0) val=2*(-val);
    else if (val>0) val=(2*val)-1;

    int Q=val>>K;
    int R=val&((1<<K)-1);

    cout << "<" << Q << "," << R << ">";
  }
  Coder.Stop();
}

void Model::EncodeFile(Wav &myWav,Sac &mySac)
{
  framesize=1*myWav.getSampleRate();

  samplesdata.resize(myWav.getNumChannels());
  for (int i=0;i<myWav.getNumChannels();i++) samplesdata[i].resize(framesize);

  samplesenc.resize(myWav.getNumChannels());

  myWav.InitReader(framesize);
  int samplescoded=0;
  int samplestocode=myWav.getNumSamples();
  while (samplestocode>0) {
    int samplesread=myWav.ReadSamples(samplesdata,framesize);

    for (int i=0;i<myWav.getNumChannels();i++) {
         PredictMonoFrame(i,samplesread);
    }
    for (int i=0;i<myWav.getNumChannels();i++) {
         EncodeMonoFrame(i,samplesread);
    }
    for (int i=0;i<myWav.getNumChannels();i++) {
         uint8_t buf[4];
         binUtils::put32LH(buf,samplesenc[i].GetBufPos());
         mySac.WriteData(samplesenc[i].GetBuf(),samplesenc[i].GetBufPos());
    }

    samplescoded+=samplesread;
    double r=samplescoded*100.0/(double)myWav.getNumSamples();
    cout << "  " << samplescoded << "/" << myWav.getNumSamples() << ":" << setw(6) << miscUtils::ConvertFixed(r,1) << "%\r";

    samplestocode-=samplesread;
  }
  cout << "\n";
}

