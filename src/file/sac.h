#ifndef SAC_H
#define SAC_H

#include "file.h"
#include "wav.h"
#include "../utils.h"

class Sac : public AudioFile
{
  public:
    Sac():AudioFile(),metadatasize(0){};
    Sac(Wav &file)
    :AudioFile(file),metadatasize(0)
    {

    }
    int WriteHeader(Wav &myWav);
    int ReadHeader();
    int UnpackMetaData(Wav &myWav);
    vector <uint8_t>metadata;
  private:
     uint32_t metadatasize;
};


#endif // SAC_H
