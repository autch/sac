#ifndef SAC_H
#define SAC_H

#include "file.h"
#include "wav.h"
#include "../utils.h"

class Sac : public AudioFile
{
  public:
    Sac():AudioFile(){};
    Sac(Wav &file)
    :AudioFile(file)
    {

    }
    int WriteHeader(Wav &myWav);
    int ReadHeader(Wav &myWav);
  private:
};


#endif // SAC_H
