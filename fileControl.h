#ifndef FILECONTROL_H_
#define FILECONTROL_H_

#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <openssl/md5.h>
#ifdef WIN32
#include <windows.h>
#include <process.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <unistd.h>
#endif

#include "defenitions.h"
#include "User.h"
class User;

typedef struct {
    bool isLoaded;
    const User* user;
    union {
        char* path;
        struct {
            char* src;
            char* dst;
        } path2;
    } data;
} fsData;

void createFSthread(threadReturnValue(*function)(void*), fsData* data, const User* user);

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
threadReturnValue symbolicLink(void* data);
#endif

threadReturnValue getContentDirectory(void* data);
threadReturnValue createDirectory(void* data);
#define moveDirectory moveFile
threadReturnValue removeFolder(void* data);
threadReturnValue copyFolder(void* data);

threadReturnValue moveFile(void* data);
threadReturnValue copyFile(void* data);
threadReturnValue removeFile(void* data);
uint64_t getFilesize(char* path, const User* user);
threadReturnValue getMD5OfFile(void* data);

#endif
