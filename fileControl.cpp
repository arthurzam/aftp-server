#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <dirent.h>
#include <openssl/md5.h>
#ifdef WIN32
#include <winsock2.h>
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

#include "fileControl.h"
#include "User.h"

extern char* base_server_folder;


bool isDirectory(const char* directory)
{
    DIR *dir = opendir(directory);
    if(!dir)
        return (false);
    closedir (dir);
    return (true);
}

bool getRealDirectory(char* realativDirectory, char* result)
{
    strcpy(result, base_server_folder);
    char* relDirP = realativDirectory - 1;
    while((relDirP = strchr(relDirP + 1, PATH_SEPERATOR_BAD)))
        *relDirP = PATH_SEPERATOR_GOOD;
    strcat(result, (realativDirectory[0] == PATH_SEPERATOR_GOOD ? realativDirectory + 1 : realativDirectory));
    return (true);
}

void getContentDirectory(fsData* data)
{
    char* base = data->data.path;

    char *dirP, *resP;
    DIR* dir = NULL;
    dirP = base + strlen(base);
    if(*(dirP - 1) != PATH_SEPERATOR_GOOD)
        *(dirP++) = PATH_SEPERATOR_GOOD;

    char result[BUFFER_SERVER_SIZE + 5];
    resP = result; // a pointer to the last character

    struct dirent *ent;
    dir = opendir (base);
    if(!dir)
        goto _exit;
    int fileNameLen;
    bool flag;
    while ((ent = readdir (dir)))
    {
        if(ent->d_name[0] == '.' && (ent->d_name[1] == 0 || (ent->d_name[1] == '.' && ent->d_name[2] == 0))) // the path is not "." or ".."
            continue;
        fileNameLen = strlen(ent->d_name);
        if(resP - result + fileNameLen >= BUFFER_SERVER_SIZE - 10)
        {
            data->user->sendData(201, result, resP - result);
            resP = result;
        }
        memcpy(dirP, ent->d_name, fileNameLen + 1);
        flag = isDirectory(base);
        if(flag)
            *(resP++) = '[';
        memcpy(resP, ent->d_name, fileNameLen);
        resP += fileNameLen;
        if(flag)
            *(resP++) = ']';
        *(resP++) = '|';
    }
    closedir (dir);
    if(resP != result)
        data->user->sendData(201, result, resP - result);
_exit:
    data->user->sendData(dir ? 200 : 300);
}

#ifndef WIN32
void symbolicLink(fsData* data)
{

    char srcT[FILENAME_MAX];
    while(readlink(data->data.path2.src, srcT, FILENAME_MAX - 1) <= 0)
        memcpy(data->data.path2.src, srcT, FILENAME_MAX);
    memcpy(data->data.path2.src, srcT, FILENAME_MAX);
    if(symlink(data->data.path2.src, data->data.path2.dst))
        data->user->sendData(300);
    else
        data->user->sendData(200);
}
#endif

void createDirectory(fsData* data)
{
#ifdef WIN32
    if(CreateDirectoryA(data->data.path, NULL))
        data->user->sendData(200);
    else
        data->user->sendData(300);
#else
    struct stat st;
    memset(&st, 0, sizeof(struct stat));
    if (stat(data->data.path, &st) == -1)
    {
        if(mkdir(data->data.path, 0700) == 0)
        {
            data->user->sendData(200);
        }
    }
    data->user->sendData(300);
#endif
}

void moveFile(fsData* data)
{
    if(rename(data->data.path2.src, data->data.path2.dst))
        data->user->sendData(300);
    else
        data->user->sendData(200);
}

void copyFile(fsData* data)
{
#ifdef WIN32
    if(CopyFileA(data->data.path2.src, data->data.path2.dst, 0))
        data->user->sendData(200);
    else
        data->user->sendData(300);
#else
    int src = open(data->data.path2.src, O_RDONLY, 0);
    int dst = open(data->data.path2.dst, O_WRONLY | O_CREAT, 0644);
    struct stat stat_source;
    fstat(src, &stat_source);
    if(sendfile(dst, src, 0, stat_source.st_size) == stat_source.st_size)
        data->user->sendData(200);
    else
        data->user->sendData(300);
    close(dst);
    close(src);
#endif
}

void removeFile(fsData* data)
{
    if(remove(data->data.path))
        data->user->sendData(300);
    else
        data->user->sendData(200);
}

// TODO: fix for 64 bit endian
void getFilesize(fsData* data)
{
    uint64_t l = (uint64_t)-1;
#ifdef WIN32
    HANDLE MF = CreateFile(data->data.path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    DWORD High;
    if ( MF != INVALID_HANDLE_VALUE )
    {
        l = GetFileSize(MF, &High);
    }
#else
    struct stat st;
    if(stat(data->data.path, &st) >= 0)
        l = st.st_size;
#endif
    if(l == (uint64_t)-1)
        data->user->sendData(300);
    else
        data->user->sendData(200, &l, sizeof(l));
}

void getMD5OfFile(fsData* data)
{
    MD5_CTX ctx;
    uint8_t result[MD5_DIGEST_LENGTH];
    int i;
    FILE* f;
    uint8_t buffer[512];
    if(!(f = fopen(data->data.path, "rb")))
    {
        data->user->sendData(300);
    }
    else
    {
        MD5_Init(&ctx);
        while((i = fread(buffer, 1, 512, f)))
            MD5_Update(&ctx, buffer, i);
        MD5_Final(result, &ctx);
        data->user->sendData(200, result, MD5_DIGEST_LENGTH);
        fclose(f);
    }
}

void removeFolder(fsData* data)
{
#ifdef WIN32
    char command[REL_PATH_MAX + BASE_FOLDER_MAX + 9] = "rd /q /s ";
    const int baseCommandLen = 9;
#else
    char command[REL_PATH_MAX + BASE_FOLDER_MAX + 9] = "rm -r -f ";
    const int baseCommandLen = 9;
#endif
    strcpy(command + baseCommandLen, data->data.path);
    if(system(command))
        data->user->sendData(300);
    else
        data->user->sendData(200);
}

void copyFolder(fsData* data)
{
#ifdef WIN32
    char command[2 * (REL_PATH_MAX + BASE_FOLDER_MAX) + 20] = "xcopy /E /H /Y /i ";
    const int baseCommandLen = 18;
#else
    char command[2 * (REL_PATH_MAX + BASE_FOLDER_MAX) + 17] = "cp -r -f ";
    const int baseCommandLen = 9;
#endif
    char* cP = command + baseCommandLen;
    strcpy(cP, data->data.path2.src);
    *(cP += strlen(cP)) = ' ';
    strcpy(cP + 1, data->data.path2.dst);

    if(system(command))
        data->user->sendData(300);
    else
        data->user->sendData(200);
}

bool isFileExists(const char* path)
{
    FILE* file = fopen(path, "rb");
    if(!file)
        return (false);
    fclose(file);
    return (true);
}
