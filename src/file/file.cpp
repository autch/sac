#include "file.h"

streampos AudioFile::readFileSize()
{
    streampos oldpos=file.tellg();
    file.seekg(0,ios_base::end);
    streampos fsize = file.tellg();
    file.seekg(oldpos);
    return fsize;
}

int AudioFile::OpenRead(const string &fname)
{
    file.open(fname,ios_base::in|ios_base::binary);
    if (file.is_open()) {filesize=readFileSize();return 0;}
    else return 1;
}

int AudioFile::OpenWrite(const string &fname)
{
  file.open(fname,ios_base::out|ios_base::binary);
  if (file.is_open()) return 0;
  else return 1;
}

