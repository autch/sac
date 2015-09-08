#ifndef FILE_H
#define FILE_H

#include "global.h"

class AudioFile {
  public:
    AudioFile():samplerate(0),bitspersample(0),numchannels(0),numsamples(0),kbps(0){};
    AudioFile(const AudioFile &file)
    :samplerate(file.getSampleRate()),bitspersample(file.getBitsPerSample()),
    numchannels(file.getNumChannels()),numsamples(file.getNumSamples()),kbps(0){};

    int OpenRead(const string &fname);
    int OpenWrite(const string &fname);
    streampos getFileSize() const {return filesize;};
    int getNumChannels()const {return numchannels;};
    int getSampleRate()const {return samplerate;};
    int getBitsPerSample()const {return bitspersample;};
    int getKBPS()const {return kbps;};
    int getNumSamples()const {return numsamples;};
    streampos readFileSize();
    void Close() {if (file.is_open()) file.close();};
  protected:
    fstream file;
    streampos filesize;
    int samplerate,bitspersample,numchannels,numsamples,kbps;
};
#endif // FILE_H
