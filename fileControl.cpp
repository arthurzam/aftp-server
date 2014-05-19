#include "fileControl.h"
extern char* base_server_folder;
#include <stdbool.h>
bool isDirectory(char* directory)
{
    DIR *dir;
    dir = opendir(directory);
    if(!dir)
        return (false);
    closedir (dir);
    return (true);
}

void createFSthread(threadReturnValue(*function)(void*), fsData* data, User* user)
{
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
    const char FILE_LIST_SEPARATOR = '|';
    char base[FILENAME_MAX];
    bool flag = data->user->getRealFile(data->data.path, base);
    data->isLoaded = true;
    char *dirP, *resP;
    DIR* dir;
    if(!flag)
        goto _exit;
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
        *(resP++) = FILE_LIST_SEPARATOR;
    }
    closedir (dir);
    if(resP != result)
        data->user->sendData(201, result, resP - result);
_exit:
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

    return NULL;
}
#endif

threadReturnValue createDirectory(void* dataV)
{
    fsData* data = (fsData*)dataV;
    char directory[FILENAME_MAX];
    bool flag = data->user->getRealFile(data->data.path, directory);
    data->isLoaded = true;
#ifdef WIN32
    if(flag && CreateDirectoryA(directory, NULL))
        data->user->sendData(200);
    else
        data->user->sendData(300);
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
    if(!flag || rename(src, dst))
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
    bool flag = data->user->getRealFile(data->data.path2.src, srcS) &&
                data->user->getRealFile(data->data.path2.dst, dstS);
    data->isLoaded = true;
#ifdef WIN32
    if(flag && CopyFileA(srcS, dstS, 0))
        data->user->sendData(200);
    else
        data->user->sendData(300);
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
    return NULL;
#endif
}

threadReturnValue removeFile(void* dataV)
{
    fsData* data = (fsData*)dataV;
    char src[FILENAME_MAX];
    bool flag = data->user->getRealFile(data->data.path, src);
    data->isLoaded = true;
    if(!flag || remove(src))
        data->user->sendData(300);
    else
        data->user->sendData(200);
#ifndef WIN32
    return NULL;
#endif
}

uint64_t getFilesize(char* path, User* user)
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
    uint8_t result[MD5_RESULT_LENGTH];
    char filePath[FILENAME_MAX];
    bool flag = data->user->getRealFile(data->data.path, filePath);
    data->isLoaded = true;

    md5_context ctx;
    int i;
    FILE* f = NULL;
    uint8_t buffer[512];

    if(!(flag && (f = fopen(filePath, "rb"))))
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
    fclose(f);
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
    if(!flag || system(command))
        data->user->sendData(300);
    else
        data->user->sendData(200);
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
    int i;
    if(!data->user->getRealFile(data->data.path2.src, command + baseCommandLen))
        goto _bad;
    i = strlen(command);
    command[i++] = ' ';
    data->user->getRealFile(data->data.path2.dst, command + i);
    data->isLoaded = true;
    if(system(command))
_bad:
        data->user->sendData(300);
    else
        data->user->sendData(200);
#ifndef WIN32
    return NULL;
#endif
}

bool isFileExists(char* path)
{
    FILE* file = fopen(path, "rb");
    if(!file)
        return (false);
    fclose(file);
    return (true);
}
