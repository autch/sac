#include "global.h"
#include "codec.h"
#include "utils.h"
#include "timer.h"
#include "file/wav.h"
#include "file/sac.h"
#include "math/cholesky.h"
#include "math/cov.h"

void PrintInfo(const AudioFile &myWav)
{
  cout << "  WAVE  Codec: PCM (" << myWav.getKBPS() << " kbps)\n";
  cout << "  " << myWav.getSampleRate() << "Hz " << myWav.getBitsPerSample() << " Bit  ";
  if (myWav.getNumChannels()==1) cout << "Mono";
  else if (myWav.getNumChannels()==2) cout << "Stereo";
  else cout << myWav.getNumChannels() << " Channels";
  cout << "\n";
  cout << "  " << myWav.getNumSamples() << " Samples [" << miscUtils::getTimeStrFromSamples(myWav.getNumSamples(),myWav.getSampleRate()) << "]\n";
}

int main(int argc,char *argv[])
{
  /*Cholesky myChol;
  cout << " test Cholesky Decomposition: ";
  if (myChol.Test()) cout << "ok" << endl;
  else cout << "error" << endl;*/

  cout << "Sac v0.0.2 - Lossless Audio Coder (c) Sebastian Lehmann\n";
  cout << "compiled on " << __DATE__ << " ";
  #ifdef __x86_64
    cout << "(64-bit)";
  #else
    cout << "(32-bit)";
  #endif
  cout << "\n\n";
  if (argc < 2) {
    cout << "usage: sac [--options] input output\n\n";
    cout << "  --encode         encode input.wav to output.sac (default)\n";
    cout << "  --fast           fast, sacrifice compression\n";
    cout << "  --normal         good compression\n";
    cout << "  --decode         decode input.sac to output.wav\n";
    return 1;
  }

  string sinputfile,soutputfile;
  int mode=0;

  bool first=true;
  string param,uparam;
  for (int i=1;i<argc;i++) {
    param=uparam=argv[i];
    strUtils::strUpper(uparam);
    if (param.length()>1 && (param[0]=='-' && param[1]=='-')) {
       if (uparam.compare("--ENCODE")==0) mode=0;
       else if (uparam.compare("--DECODE")==0) mode=1;
       else cout << "warning: unknown option '" << param << "'\n";
    } else {
       if (first) {sinputfile=param;first=false;}
       else soutputfile=param;
    }
  }
  Timer myTimer;
  myTimer.start();

  if (mode==0) {
    Wav myWav(true);
    cout << "Open: '" << sinputfile << "': ";
    if (myWav.OpenRead(sinputfile)==0) {
      cout << "ok (" << myWav.getFileSize() << " Bytes)\n";
      if (myWav.ReadHeader()==0) {
         Sac mySac(myWav);
         cout << "Create: '" << soutputfile << "': ";
         if (mySac.OpenWrite(soutputfile)==0) {
           cout << "ok\n";
           PrintInfo(myWav);
           Codec myCodec;
           myCodec.EncodeFile(myWav,mySac);
           uint64_t infilesize=myWav.getFileSize();
           uint64_t outfilesize=mySac.readFileSize();
           double r=0.;
           if (outfilesize) r=outfilesize*100./infilesize;
           cout << endl << "  " << infilesize << "->" << outfilesize<< "=" <<miscUtils::ConvertFixed(r,2) << "%"<<endl;
           mySac.Close();
         } else cout << "could not create\n";
      } else cout << "warning: input is not a valid .wav file\n";
      myWav.Close();
    } else cout << "could not open\n";
  } else if (mode==1) {
    cout << "Open: '" << sinputfile << "': ";
    Sac mySac;
    if (mySac.OpenRead(sinputfile)==0) {
      cout << "ok (" << mySac.getFileSize() << " Bytes)\n";
      if (mySac.ReadHeader()==0) {
           PrintInfo(mySac);
           Wav myWav(mySac,true);
           cout << "Create: '" << soutputfile << "': ";
           if (myWav.OpenWrite(soutputfile)==0) {
             cout << "ok\n";
             Codec myCodec;
             myCodec.DecodeFile(mySac,myWav);
             myWav.Close();
           } else cout << "could not create\n";
      } else cout << "warning: input is not a valid .sac file\n";
      mySac.Close();
    }
  }

  myTimer.stop();
  cout << "  Time:   [" << miscUtils::getTimeStrFromSeconds(round(myTimer.elapsedS())) << "]" << endl;

  return 0;
}
