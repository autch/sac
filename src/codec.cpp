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
  BufIO &buf=samplesenc[ch];
  buf.Reset();

  RangeCoderSH rc(buf);
  rc.Init();

  Rice ricecoder(rc);

  for (int i=0;i<numsamples;i++) ricecoder.Encode(data[i]);
  rc.Stop();
}

void Model::DecodeMonoFrame(int ch,int numsamples)
{
  int32_t *data=&(samplestemp[ch][0]);
  BufIO &buf=samplesenc[ch];
  buf.Reset();

  RangeCoderSH rc(buf,1);
  rc.Init();

  Rice ricecoder(rc);

  for (int i=0;i<numsamples;i++) data[i]=ricecoder.Decode();
  rc.Stop();
}


void Model::EncodeFile(Wav &myWav,Sac &mySac)
{
  framesize=1*myWav.getSampleRate();

  samplesdata.resize(myWav.getNumChannels());
  for (int i=0;i<myWav.getNumChannels();i++) samplesdata[i].resize(framesize);

  samplestemp.resize(myWav.getNumChannels());
  for (int i=0;i<myWav.getNumChannels();i++) samplestemp[i].resize(framesize);


  samplesenc.resize(myWav.getNumChannels());

  mySac.WriteHeader();
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
         DecodeMonoFrame(i,samplesread);
         for (int j=0;j<samplesread;j++)
         {
            if (samplesdata[i][j]!=samplestemp[i][j]) cout << "error: decoded sample: " << samplesdata[i][j] << " " << samplestemp[i][j] << endl;
         }
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

