#include "global.h"
#include "codec.h"
#include "common/timer.h"

void PrintInfo(const AudioFile &myWav)
{
  std::cout << "  WAVE  Codec: PCM (" << myWav.getKBPS() << " kbps)\n";
  std::cout << "  " << myWav.getSampleRate() << "Hz " << myWav.getBitsPerSample() << " Bit  ";
  if (myWav.getNumChannels()==1) std::cout << "Mono";
  else if (myWav.getNumChannels()==2) std::cout << "Stereo";
  else std::cout << myWav.getNumChannels() << " Channels";
  std::cout << "\n";
  std::cout << "  " << myWav.getNumSamples() << " Samples [" << miscUtils::getTimeStrFromSamples(myWav.getNumSamples(),myWav.getSampleRate()) << "]\n";
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
  std::cout << "Sac v0.0.4 - Lossless Audio Coder (c) Sebastian Lehmann\n";
  std::cout << "compiled on " << __DATE__ << " ";
  #ifdef __x86_64
    std::cout << "(64-bit)";
  #else
    std::cout << "(32-bit)";
  #endif
  std::cout << "\n\n";
  if (argc < 2) {
    std::cout << "usage: sac [--options] input output\n\n";
    std::cout << "  --encode         encode input.wav to output.sac (default)\n";
    std::cout << "  --fast           fast, sacrifice compression\n";
    std::cout << "  --normal         normal compression (default)\n";
    std::cout << "  --high           high compression, slow\n";
    std::cout << "  --decode         decode input.sac to output.wav\n";
    return 1;
  }

  std::string sinputfile,soutputfile;
  int mode=0;
  int profile=1;

  bool first=true;
  std::string param,uparam;
  for (int i=1;i<argc;i++) {
    param=uparam=argv[i];
    strUtils::strUpper(uparam);
    if (param.length()>1 && (param[0]=='-' && param[1]=='-')) {
       if (uparam.compare("--ENCODE")==0) mode=0;
       else if (uparam.compare("--DECODE")==0) mode=1;
       else if (uparam.compare("--FAST")==0) profile=0;
       else if (uparam.compare("--NORMAL")==0) profile=1;
       else if (uparam.compare("--HIGH")==0) profile=2;
       else std::cout << "warning: unknown option '" << param << "'\n";
    } else {
       if (first) {sinputfile=param;first=false;}
       else soutputfile=param;
    }
  }
  Timer myTimer;
  myTimer.start();

  if (mode==0) {
    Wav myWav(true);
    std::cout << "Open: '" << sinputfile << "': ";
    if (myWav.OpenRead(sinputfile)==0) {
      std::cout << "ok (" << myWav.getFileSize() << " Bytes)\n";
      if (myWav.ReadHeader()==0) {
         if (myWav.getBitsPerSample()!=16 || myWav.getNumChannels()!=2) {
            std::cout << "Unsupported input format." << std::endl;
            myWav.Close();
            return 1;
         }
         Sac mySac(myWav);
         std::cout << "Create: '" << soutputfile << "': ";
         if (mySac.OpenWrite(soutputfile)==0) {
           std::cout << "ok\n";
           PrintInfo(myWav);
           std::cout << "  Profile: ";
           if (profile==0) std::cout << "fast" << std::endl;
           else if (profile==1) std::cout << "normal" << std::endl;
           else if (profile==2) std::cout << "high" << std::endl;
           Codec myCodec;
           myCodec.EncodeFile(myWav,mySac,profile);
           uint64_t infilesize=myWav.getFileSize();
           uint64_t outfilesize=mySac.readFileSize();
           double r=0.,bps=0.;
           if (outfilesize) {
             r=outfilesize*100./infilesize;
             bps=(outfilesize*8.)/static_cast<double>(myWav.getNumSamples()*myWav.getNumChannels());
           }
           std::cout << std::endl << "  " << infilesize << "->" << outfilesize<< "=" <<miscUtils::ConvertFixed(r,2) << "% (" << miscUtils::ConvertFixed(bps,2)<<" bps)"<<std::endl;

           mySac.Close();
         } else std::cout << "could not create\n";
      } else std::cout << "warning: input is not a valid .wav file\n";
      myWav.Close();
    } else std::cout << "could not open\n";
  } else if (mode==1) {
    std::cout << "Open: '" << sinputfile << "': ";
    Sac mySac;
    if (mySac.OpenRead(sinputfile)==0) {
      std::cout << "ok (" << mySac.getFileSize() << " Bytes)\n";
      if (mySac.ReadHeader()==0) {
           PrintInfo(mySac);
           Wav myWav(mySac,true);
           std::cout << "Create: '" << soutputfile << "': ";
           if (myWav.OpenWrite(soutputfile)==0) {
             std::cout << "ok\n";
             std::cout << "  Profile: ";
             if (mySac.GetProfile()==0) std::cout << "fast" << std::endl;
             else if (mySac.GetProfile()==1) std::cout << "normal" << std::endl;
             else if (mySac.GetProfile()==1) std::cout << "high" << std::endl;

             Codec myCodec;
             myCodec.DecodeFile(mySac,myWav);
             myWav.Close();
           } else std::cout << "could not create\n";
      } else std::cout << "warning: input is not a valid .sac file\n";
      mySac.Close();
    }
  }

  myTimer.stop();
  std::cout << "  Time:   [" << miscUtils::getTimeStrFromSeconds(round(myTimer.elapsedS())) << "]" << std::endl;

  return 0;
}
