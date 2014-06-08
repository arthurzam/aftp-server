#include <cstdio>
#include <cstring>
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

extern char* base_server_folder;

#define MAXIMUM_IO_THREADS 0x8F
int runningIOthreads = 0;

bool isDirectory(const char* directory)
{
    DIR *dir = opendir(directory);
    if(!dir)
        return (false);
    closedir (dir);
    return (true);
}

void createFSthread(threadReturnValue(*function)(void*), fsData* data, const User* user)
{
    if(runningIOthreads >= MAXIMUM_IO_THREADS)
    {
        user->sendData(302);
        return;
    }

    data->isLoaded = false;
    data->user = user;
#ifdef WIN32
    _beginthread(function, 0, data);
#else
    pthread_t thread;
    pthread_create(&thread, NULL, function, data);
#endif
    while(!data->isLoaded);
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

threadReturnValue getContentDirectory(void* dataV)
{
    fsData* data = (fsData*)dataV;
    char base[FILENAME_MAX];
    bool flag = data->user->getRealFile(data->data.path, base);
    data->isLoaded = true;

    char *dirP, *resP;
    DIR* dir = NULL;
    if(!flag)
        goto _exit;
    dirP = base + strlen(base);
    if(*(dirP - 1) != PATH_SEPERATOR_GOOD)
        *(dirP++) = PATH_SEPERATOR_GOOD;

    char result[BUFFER_SERVER_SIZE + 5];
    resP = result; // a pointer to the last character

    struct dirent *ent;
    ++runningIOthreads;
    dir = opendir (base);
    if(!dir)
        goto _exit;
    int fileNameLen;
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
    --runningIOthreads;
    data->user->sendData(dir ? 200 : 300);
#ifndef WIN32
    return NULL;
#endif
}

#ifndef WIN32
threadReturnValue symbolicLink(void* dataV)
{
    fsData* data = (fsData*)dataV;
    char src[FILENAME_MAX], dst[FILENAME_MAX];
    bool flag = data->user->getRealFile(data->data.path2.src, src) &&
                data->user->getRealFile(data->data.path2.dst, dst);
    data->isLoaded = true;

    char srcT[FILENAME_MAX];
    ++runningIOthreads;
    if(flag)
    {
        while(readlink(src, srcT, FILENAME_MAX - 1) <= 0)
            memcpy(src, srcT, FILENAME_MAX);
        memcpy(src, srcT, FILENAME_MAX);
    }
    if(!flag || symlink(src, dst))
        data->user->sendData(300);
    else
        data->user->sendData(200);
    --runningIOthreads;
    return NULL;
}
#endif

threadReturnValue createDirectory(void* dataV)
{
    fsData* data = (fsData*)dataV;
    char directory[FILENAME_MAX];
    bool flag = data->user->getRealFile(data->data.path, directory);
    data->isLoaded = true;
    ++runningIOthreads;
#ifdef WIN32
    if(flag && CreateDirectoryA(directory, NULL))
        data->user->sendData(200);
    else
        data->user->sendData(300);
    --runningIOthreads;
#else
    struct stat st = {0};
    if (flag && stat(directory, &st) == -1)
    {
        if(mkdir(directory, 0700) == 0)
        {
            data->user->sendData(200);
            return NULL;
        }
    }
    data->user->sendData(300);
    --runningIOthreads;
    return NULL;
#endif
}

threadReturnValue moveFile(void* dataV)
{
    fsData* data = (fsData*)dataV;
    char src[FILENAME_MAX], dst[FILENAME_MAX];
    bool flag = data->user->getRealFile(data->data.path2.src, src) &&
                data->user->getRealFile(data->data.path2.dst, dst);
    data->isLoaded = true;
    ++runningIOthreads;
    if(!flag || rename(src, dst))
        data->user->sendData(300);
    else
        data->user->sendData(200);
    --runningIOthreads;
#ifndef WIN32
    return NULL;
#endif
}

threadReturnValue copyFile(void* dataV)
{
    fsData* data = (fsData*)dataV;
    char srcS[FILENAME_MAX], dstS[FILENAME_MAX];
    bool flag = data->user->getRealFile(data->data.path2.src, srcS) &&
                data->user->getRealFile(data->data.path2.dst, dstS);
    data->isLoaded = true;
    ++runningIOthreads;
#ifdef WIN32
    if(flag && CopyFileA(srcS, dstS, 0))
        data->user->sendData(200);
    else
        data->user->sendData(300);
    --runningIOthreads;
#else
    if(!flag)
        data->user->sendData(300);
    int src = open(srcS, O_RDONLY, 0);
    int dst = open(dstS, O_WRONLY | O_CREAT, 0644);
    struct stat stat_source;
    fstat(src, &stat_source);
    if(sendfile(dst, src, 0, stat_source.st_size) == stat_source.st_size)
        data->user->sendData(200);
    else
        data->user->sendData(300);
    close(dst);
    close(src);
    --runningIOthreads;
    return NULL;
#endif
}

threadReturnValue removeFile(void* dataV)
{
    fsData* data = (fsData*)dataV;
    char src[FILENAME_MAX];
    bool flag = data->user->getRealFile(data->data.path, src);
    data->isLoaded = true;
    ++runningIOthreads;
    if(!flag || remove(src))
        data->user->sendData(300);
    else
        data->user->sendData(200);
    --runningIOthreads;
#ifndef WIN32
    return NULL;
#endif
}

uint64_t getFilesize(char* path, const User* user)
{
    char src[FILENAME_MAX];
    if(!user->getRealFile(path, src))
        return (-1);
#ifdef WIN32
    HANDLE MF = CreateFile(src, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    DWORD High;
    if ( MF != INVALID_HANDLE_VALUE )
    {
        return (GetFileSize(MF, &High));
    }
#else
    struct stat st;
    if(stat(src, &st) >= 0)
        return (st.st_size);
#endif
    return (-1);
}

threadReturnValue getMD5OfFile(void* dataV)
{
    fsData* data = (fsData*)dataV;
    uint8_t result[MD5_DIGEST_LENGTH];
    char filePath[FILENAME_MAX];
    bool flag = data->user->getRealFile(data->data.path, filePath);
    data->isLoaded = true;

    MD5_CTX ctx;
    int i;
    FILE* f;
    uint8_t buffer[512];

    ++runningIOthreads;
    if(!flag || !(f = fopen(filePath, "rb")))
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
    --runningIOthreads;
#ifndef WIN32
    return NULL;
#endif
}

threadReturnValue removeFolder(void* dataV)
{
#ifdef WIN32
    char command[FILENAME_MAX + 9] = "rd /q /s ";
    const int baseCommandLen = 9;
#else
    char command[FILENAME_MAX + 9] = "rm -r -f ";
    const int baseCommandLen = 9;
#endif
    fsData* data = (fsData*)dataV;
    bool flag = data->user->getRealFile(data->data.path, command + baseCommandLen);
    data->isLoaded = true;
    ++runningIOthreads;
    if(!flag || system(command))
        data->user->sendData(300);
    else
        data->user->sendData(200);
    --runningIOthreads;
#ifndef WIN32
    return NULL;
#endif
}

threadReturnValue copyFolder(void* dataV)
{
#ifdef WIN32
    char command[2 * FILENAME_MAX + 20] = "xcopy /E /H /Y /i ";
    const int baseCommandLen = 18;
#else
    char command[2 * FILENAME_MAX + 17] = "cp -r -f ";
    const int baseCommandLen = 9;
#endif
    fsData* data = (fsData*)dataV;
    char* cP = command + baseCommandLen;
    bool flag = data->user->getRealFile(data->data.path2.src, cP);
    if(flag)
    {
        *(cP += strlen(cP)) = ' ';
        flag = data->user->getRealFile(data->data.path2.dst, cP + 1);
    }
    data->isLoaded = true;
    ++runningIOthreads;
    if(!flag || system(command))
        data->user->sendData(300);
    else
        data->user->sendData(200);
    --runningIOthreads;
#ifndef WIN32
    return NULL;
#endif
}

bool isFileExists(const char* path)
{
    FILE* file = fopen(path, "rb");
    if(!file)
        return (false);
    fclose(file);
    return (true);
}
