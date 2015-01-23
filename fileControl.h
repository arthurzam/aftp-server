#ifndef FILECONTROL_H_
#define FILECONTROL_H_

#include "defenitions.h"
#include "IOThreadPool.h"

class User;
extern "C" {
    /*
     * returns the real folder of the given path, in the result array
     */
    bool getRealDirectory(char* realativDirectory, char* result);
    bool isDirectory(const char* directory);
    bool isFileExists(const char* path);

#ifndef WIN32
    /*
     * only for Unix based OS
     */
    void symbolicLink(fsData* data);
#endif

    void getContentDirectory(fsData* data);
    void createDirectory(fsData* data);
#define moveDirectory moveFile
    void removeFolder(fsData* data);
    void copyFolder(fsData* data);

    void moveFile(fsData* data);
    void copyFile(fsData* data);
    void removeFile(fsData* data);
    void getFilesize(fsData* data);
    void getMD5OfFile(fsData* data);
}
#endif
