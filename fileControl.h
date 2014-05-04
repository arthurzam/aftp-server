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

bool_t isDirectory(char* directory);
/*
 * returns the real folder of the given path, in an allocated char array
 * for example, from realativDirectory=/aaa\a will return {BASE_FOLDER}/aaa/a
 * if result is given, the result would be copied into the array and return NULL in success
 */
char* getRealDirectory(char* realativDirectory, char* result);
bool_t getContentDirectory(char* directory, User* user);
bool_t createDirectory(char* directory, User* user);

bool_t moveFile  (char* from, char* to, User* user);
bool_t copyFile  (char* from, char* to, User* user);
bool_t removeFile(char* path, User* user);
unsigned long long int getFilesize(char* path, User* user);
bool_t getMD5OfFile(char* path, User* user, byte_t result[MD5_RESULT_LENGTH]);

#define moveDirectory moveFile
bool_t removeFolder(char* path, User* user);
bool_t copyFolder(char* from, char* to, User* user);

bool_t isFileExists(char* path);

#endif
