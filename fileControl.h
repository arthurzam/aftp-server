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
    int symbolicLink(fsData* data);
#endif

    int getContentDirectory(fsData* data);
    int createDirectory(fsData* data);
#define moveDirectory moveFile
    int removeFolder(fsData* data);
    int copyFolder(fsData* data);

    int moveFile(fsData* data);
    int copyFile(fsData* data);
    int removeFile(fsData* data);
    int getFileStat(fsData* data);
    int getMD5OfFile(fsData* data);
}
#endif
