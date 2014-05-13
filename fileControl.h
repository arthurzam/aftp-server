#ifndef FILECONTROL_H_
#define FILECONTROL_H_

#include <cstdio>
#include <cstring>
#include <dirent.h>
#ifdef WIN32
#include <windows.h>
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
#include "md5.h"
class User;

typedef struct{
    bool_t isLoaded;
    User* user;
    union {
        char* path;
        struct{
            char* src;
            char* dst;
        } path2;
    } data;
} fsData;

void createFSthread(threadReturnValue(*function)(void*), fsData* data, User* user);

/*
 * returns the real folder of the given path, in the result array
 */
bool_t getRealDirectory(char* realativDirectory, char* result);
bool_t isDirectory(char* directory);
bool_t isFileExists(char* path);

threadReturnValue symbolicLink(void* data);

threadReturnValue getContentDirectory(void* data);
threadReturnValue createDirectory(void* data);
#define moveDirectory moveFile
threadReturnValue removeFolder(void* data);
threadReturnValue copyFolder(void* data);

threadReturnValue moveFile(void* data);
threadReturnValue copyFile(void* data);
threadReturnValue removeFile(void* data);
unsigned long long int getFilesize(char* path, User* user);
threadReturnValue getMD5OfFile(void* data);

#endif
