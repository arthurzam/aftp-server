#ifndef FILECONTROL_H_
#define FILECONTROL_H_

#include <dirent.h>
#include "defenitions.h"
#include "User.h"
#include "md5.h"
class User;
#include <string>
using namespace std;

#ifdef WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <unistd.h>
#endif

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
 * returns the real folder of the given path, in an allocated char array
 * for example, from realativDirectory=/aaa\a will return {BASE_FOLDER}/aaa/a
 * if result is given, the result would be copied into the array and return NULL in success
 */
char* getRealDirectory(char* realativDirectory, char* result);
bool_t isDirectory(char* directory);
bool_t isFileExists(char* path);

bool_t getContentDirectory(char* directory, User* user);
bool_t createDirectory(char* directory, User* user);
#define moveDirectory moveFile
threadReturnValue removeFolder(void* data);
bool_t copyFolder(char* from, char* to, User* user);

threadReturnValue moveFile(void* data);
threadReturnValue copyFile(void* data);
threadReturnValue removeFile(void* data);
unsigned long long int getFilesize(char* path, User* user);
threadReturnValue getMD5OfFile(void* data);

#endif
