#ifndef SAC_H
#define SAC_H

#include "file.h"
#include "utils.h"

class Sac : public AudioFile
{
  public:
    using AudioFile::AudioFile;
    Sac(const AudioFile &file)
    :AudioFile(file)
    {

    }
    int WriteHeader();
};


#endif // SAC_H
