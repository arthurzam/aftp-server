#include "fileControl.h"
#include "defenitions.h"
#include <cstdio>
#include <cstring>

bool_t isDirectory(char* directory)
{
    DIR *dir;
    dir = opendir(directory);
    if(!dir)
        return (FALSE);
    closedir (dir);
    return (TRUE);
}

void createFSthread(threadReturnValue(*function)(void*), fsData* data, User* user)
{
    data->isLoaded = FALSE;
    data->user = user;
#ifdef WIN32
    _beginthread(function, 0, data);
#else
    pthread_t thread;
    pthread_create(&thread, NULL, function, data);
#endif
    while(!data->isLoaded);
}

char* getRealDirectory(char* realativDirectory, char* result)
{
    char* realDirectory;
    if(result)
        realDirectory = result;
    else
        realDirectory = (char*)malloc(FILENAME_MAX);
    int i;
    strcpy(realDirectory, SERVER_BASE_FOLDER);
    for(i = 0; realativDirectory[i]; ++i)
        if(realativDirectory[i] == PATH_SEPERATOR_BAD)
            realativDirectory[i] = PATH_SEPERATOR_GOOD;
    strcat(realDirectory, (*realativDirectory == PATH_SEPERATOR_GOOD ? realativDirectory + 1 : realativDirectory));
    return (result ? NULL : realDirectory);
}

bool_t getContentDirectory(char* directory, User* user)
{
    const char FILE_LIST_SEPARATOR = '|';
    char base[FILENAME_MAX];
    user->getRealFile(directory, base);
    int dirLen = strlen(base);
    if(base[dirLen - 1] != PATH_SEPERATOR_GOOD)
    {
        base[dirLen++] = PATH_SEPERATOR_GOOD;
        base[dirLen] = 0;
    }

    int resLen = 0;
    char result[BUFFER_SERVER_SIZE + 5] = {0};
    result[BUFFER_SERVER_SIZE + 4] = 0; // just to be safe from BUFFER_OVERFLOW

    DIR *dir;
    struct dirent *ent;
    dir = opendir (base);
    if(!dir)
        return (FALSE);
    bool_t flag;
    int fileNameLen;
    while ((ent = readdir (dir)) != NULL)
    {
        if(ent->d_name[0] == '.' && (ent->d_name[1] == 0 || (ent->d_name[1] == '.' && ent->d_name[2] == 0))) // the path is not "." or ".."
            continue;
        fileNameLen = strlen(ent->d_name);
        if(resLen + fileNameLen >= BUFFER_SERVER_SIZE - 10)
        {
            user->sendData(201, result, resLen);
            result[0] = 0;
            resLen = 0;
        }
        strcpy(base + dirLen, ent->d_name);
        flag = isDirectory(base);
        if(flag)
        {
            result[resLen++] = '[';
            result[resLen] =  0;
        }
        strcat(result + resLen, ent->d_name);
        resLen += fileNameLen;
        if(flag)
        {
            result[resLen++] = ']';
            result[resLen] =  0;
        }
        result[resLen++] = FILE_LIST_SEPARATOR;
        result[resLen] =  0;
    }
    closedir (dir);
    if(resLen)
        user->sendData(201, result, resLen);
    return (TRUE);
}

bool_t createDirectory(char* realativDirectory, User* user)
{
    char directory[FILENAME_MAX];
    user->getRealFile(realativDirectory, directory);
#ifdef WIN32
    if(CreateDirectory(directory, NULL))
    {
        return (TRUE);
    }
    return (FALSE);
#else
    struct stat st = {0};
    if (stat(directory, &st) == -1)
    {
        if(mkdir(directory, 0700) == 0)
        {
            return (TRUE);
        }
    }
    return (FALSE);
#endif
}

threadReturnValue moveFile(void* dataV)
{
    fsData* data = (fsData*)dataV;
    char src[FILENAME_MAX], dst[FILENAME_MAX];
    data->user->getRealFile(data->data.path2.src, src);
    data->user->getRealFile(data->data.path2.dst, dst);
    if(rename(src, dst))
        data->user->sendData(300);
    else
        data->user->sendData(200);
#ifndef WIN32
    return NULL;
#endif
}

threadReturnValue copyFile(void* dataV)
{
    fsData* data = (fsData*)dataV;
    char srcS[FILENAME_MAX], dstS[FILENAME_MAX];
    data->user->getRealFile(data->data.path2.src, srcS);
    data->user->getRealFile(data->data.path2.dst, dstS);
    data->isLoaded = TRUE;
#ifdef WIN32
    if(CopyFileA(srcS, dstS, 0))
        data->user->sendData(200);
    else
        data->user->sendData(300);
#else
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
    return NULL;
#endif
}

threadReturnValue removeFile(void* dataV)
{
    fsData* data = (fsData*)dataV;
    char src[FILENAME_MAX];
    data->user->getRealFile(data->data.path, src);
    data->isLoaded = TRUE;
    if(remove(src))
        data->user->sendData(300);
    else
        data->user->sendData(200);
#ifndef WIN32
    return NULL;
#endif
}

unsigned long long int getFilesize(char* path, User* user)
{
    char src[FILENAME_MAX];
    user->getRealFile(path, src);
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
    byte_t result[MD5_RESULT_LENGTH];
    char filePath[FILENAME_MAX];
    data->user->getRealFile(data->data.path, filePath);
    data->isLoaded = TRUE;

    md5_context ctx;
    int i;
    byte_t buffer[512];

    FILE* f = fopen(filePath, "rb");
    if(!f)
    {
        data->user->sendData(300);
    }
    else
    {
        md5_init(&ctx);
        while((i = fread(buffer, 1, 512, f)))
        {
            md5_append(&ctx, buffer, i);
        }
        md5_finish(&ctx, result);
        data->user->sendData(200, result, MD5_RESULT_LENGTH);
    }
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
    data->user->getRealFile(data->data.path, command + baseCommandLen);
    data->isLoaded = TRUE;
    if(system(command))
        data->user->sendData(300);
    else
        data->user->sendData(200);
#ifndef WIN32
    return NULL;
#endif
}

bool_t copyFolder(char* from, char* to, User* user)
{
#ifdef WIN32
    char command[2 * FILENAME_MAX + 20] = "xcopy /E /H /Y /i ";
    const int baseCommandLen = 18;
#else
    char command[2 * FILENAME_MAX + 17] = "cp -r -f ";
    const int baseCommandLen = 9;
#endif
    int i;
    user->getRealFile(from, command + baseCommandLen);
    i = strlen(command);
    command[i++] = ' ';
    user->getRealFile(to, command + i);
    return (system (command) == 0);
}

bool_t isFileExists(char* path)
{
    FILE* file = fopen(path, "rb");
    if(!file)
        return (FALSE);
    fclose(file);
    return (TRUE);
}
