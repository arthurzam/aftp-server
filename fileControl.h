#ifndef FILECONTROL_H_
#define FILECONTROL_H_

#include <dirent.h>
#include "defenitions.h"
#include "User.h"
class User;
#include <string>
using namespace std;

#ifdef WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#endif

bool_t isDirectory(char* directory);
/*
 * returns the real folder of the given path, in an allocated char array
 * for example, from realativDirectory=/aaa\a will return {BASE_FOLDER}/aaa/a
 */
char* getRealDirectory(char* realativDirectory);
string* getContentDirectory(char* directory);
bool_t createDirectory(char* realativDirectory);

bool_t moveFile  (char* from, char* to, User* user);
bool_t copyFile  (char* from, char* to, User* user);
bool_t removeFile(char* path, User* user);

#define moveDirectory moveFile
bool_t removeFolder(char* path, User* user);
bool_t copyFolder(char* from, char* to, User* user);

bool_t isFileExists(char* path);

#endif
