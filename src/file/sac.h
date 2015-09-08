#ifndef SAC_H
#define SAC_H

#include "file.h"
#include "wav.h"
#include "../utils.h"

class Sac : public AudioFile
{
  public:
    Sac(Wav &file)
    :AudioFile(file),myChunks(file.getChunks())
    {

    }
    int WriteHeader();
  private:
    Chunks &myChunks;
};


#endif // SAC_H
