#ifndef FILECONTROL_H_
#define FILECONTROL_H_

#include <dirent.h>
#include "defenitions.h"
#include <string>
using namespace std;

#ifdef WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#endif

bool_t isDirectory(char* directory);
char* getRealDirectory(char* realativDirectory);
string* getContentDirectory(char* directory);
bool_t createDirectory(char* realativDirectory);

bool_t moveFile(char* from, char* to);
bool_t copyFile(char* from, char* to);
void removeFile(char* path);

bool_t isFileExists(char* path);

#endif
