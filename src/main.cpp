#include "global.h"
#include "wav.h"
#include "sac.h"
#include "coder.h"
#include "utils.h"

void PrintInfo(const Wav &myWav)
{
  cout << "  WAVE  Codec: PCM (" << myWav.getKBPS() << " kbps)\n";
  cout << "  " << myWav.getSampleRate() << "Hz " << myWav.getBitsPerSample() << " Bit  ";
  if (myWav.getNumChannels()==1) cout << "Mono";
  else if (myWav.getNumChannels()==2) cout << "Stereo";
  else cout << myWav.getNumChannels() << "Channels";
  cout << "\n";
  cout << "  " << myWav.getNumSamples() << " Samples [" << miscUtils::getTimeFromSamples(myWav.getNumSamples(),myWav.getSampleRate()) << "]\n";
}

int main(int argc,char *argv[])
{
  cout << "Sac v0.0.1 - Lossless Audio Coder (c) Sebastian Lehmann\n";
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
  if (mode==0) {
    Wav myWav;
    cout << "Open: '" << sinputfile << "': ";
    if (myWav.OpenRead(sinputfile)==0) {
      cout << "ok (" << myWav.getFileSize() << " Bytes)\n";
      if (myWav.ReadHeader()==0) {
         Sac mySac(myWav);
         cout << "Create: '" << soutputfile << "': ";
         if (mySac.OpenWrite(soutputfile)==0) {
           cout << "ok\n";
           mySac.WriteHeader();
           PrintInfo(myWav);
           Coder myCoder;
           myCoder.EncodeFile(myWav);
           cout << mySac.readFileSize() << " Bytes\n";
           mySac.Close();
         } else cout << "could not create\n";
      } else cout << "warning: input is not a valid wave file\n";
      myWav.Close();
    } else cout << "could not open\n";
  } else if (mode==1) {
  }
  return 0;
}
