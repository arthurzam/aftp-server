#ifndef FILECONTROL_H_
#define FILECONTROL_H_

#include "defenitions.h"
#include "IOThreadPool.h"

extern "C" {
    inline void replaceSeperator(char* path)
    {
        while((path = strchr(path + 1, PATH_SEPERATOR_BAD)))
            *path = PATH_SEPERATOR_GOOD;
    }

    /*
     * returns the real folder of the given path, in the result array
     */
    bool getRealDirectory(char* realativDirectory, char* result);
    bool isDirectory(const char* directory);
    bool isFileExists(const char* path);

#ifdef WIN32
#define symbolicLink copyFile
#else
    bool symbolicLink(fsData* data);
#endif

    bool getContentDirectory(fsData* data);
    bool createDirectory(fsData* data);
#define moveDirectory moveFile
    bool removeFolder(fsData* data);
    bool copyFolder(fsData* data);

    bool moveFile(fsData* data);
    bool copyFile(fsData* data);
    bool removeFile(fsData* data);
    bool getFileStat(fsData* data);
    bool getMD5OfFile(fsData* data);
}
#endif
