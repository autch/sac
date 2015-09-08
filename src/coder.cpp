#include "coder.h"

void Coder::EncodeFile(Wav &myWav)
{
  framesize=1*myWav.getSampleRate();
  samplesdata.resize(myWav.getNumChannels());
  for (int i=0;i<myWav.getNumChannels();i++) samplesdata[i].resize(framesize);

  myWav.InitReader(framesize);
  int samplescoded=0;
  while (1) {
    int samplesread=myWav.ReadSamples(samplesdata,framesize);
    samplescoded+=samplesread;
    double r=samplescoded*100.0/(double)myWav.getNumSamples();
    cout << "  " << samplescoded << "/" << myWav.getNumSamples() << ":" << setw(6) << miscUtils::ConvertFixed(r,1) << "%\r";
    if (samplesread<framesize) break;
  }
  cout << "\n";
}

