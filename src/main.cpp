#include "global.h"
#include "codec.h"
#include "utils.h"
#include "common/timer.h"
#include "file/wav.h"
#include "file/sac.h"
#include "math/cholesky.h"
#include "math/cov.h"
#include "pred/rbf.h"

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

class DDS {
  public:
    DDS(int maxk)
    :maxk(maxk)
    {

    };
  private:
    int maxk;
};




class Func2D {
  public:
    Func2D(){hist[0]=hist[1]=1;};
    double Next() {
      double t=exp(-hist[0]*hist[0]);
      double r=(0.8-0.5*t)*hist[0]-
      (0.3+0.9*t)*hist[1]+0.1*sin(3.1415*hist[0]);

      hist[1]=hist[0];hist[0]=r;
      return r;
    }
  double hist[2];
};

int main(int argc,char *argv[])
{
  cout << "Sac v0.0.3 - Lossless Audio Coder (c) Sebastian Lehmann\n";
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
    //cout << "  --fast           fast, sacrifice compression\n";
    //cout << "  --normal         normal compression (default)\n";
    //cout << "  --high           high compression, slow\n";
    cout << "  --decode         decode input.sac to output.wav\n";
    return 1;
  }

  string sinputfile,soutputfile;
  int mode=0;
  int profile=1;

  bool first=true;
  string param,uparam;
  for (int i=1;i<argc;i++) {
    param=uparam=argv[i];
    strUtils::strUpper(uparam);
    if (param.length()>1 && (param[0]=='-' && param[1]=='-')) {
       if (uparam.compare("--ENCODE")==0) mode=0;
       else if (uparam.compare("--DECODE")==0) mode=1;
       else if (uparam.compare("--FAST")==0) profile=0;
       else if (uparam.compare("--NORMAL")==0) profile=1;
       else if (uparam.compare("--HIGH")==0) profile=2;
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
         if (myWav.getBitsPerSample()!=16 || myWav.getNumChannels()!=2) {
            cout << "Unsupported input format." << endl;
            myWav.Close();
            return 1;
         }
         Sac mySac(myWav);
         cout << "Create: '" << soutputfile << "': ";
         if (mySac.OpenWrite(soutputfile)==0) {
           cout << "ok\n";
           PrintInfo(myWav);
           cout << "  Profile: ";
           if (profile==0) cout << "fast" << endl;
           else if (profile==1) cout << "normal" << endl;
           else if (profile==2) cout << "high" << endl;
           Codec myCodec;
           myCodec.EncodeFile(myWav,mySac,profile);
           uint64_t infilesize=myWav.getFileSize();
           uint64_t outfilesize=mySac.readFileSize();
           double r=0.,bps=0.;
           if (outfilesize) {
             r=outfilesize*100./infilesize;
             bps=(outfilesize*8.)/static_cast<double>(myWav.getNumSamples()*myWav.getNumChannels());
           }
           cout << endl << "  " << infilesize << "->" << outfilesize<< "=" <<miscUtils::ConvertFixed(r,2) << "% (" << miscUtils::ConvertFixed(bps,2)<<" bps)"<<endl;

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
             cout << "  Profile: ";
             if (mySac.GetProfile()==0) cout << "fast" << endl;
             else if (mySac.GetProfile()==1) cout << "normal" << endl;
             else if (mySac.GetProfile()==1) cout << "high" << endl;

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
