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
#include <ftw.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include "fileControl.h"
#include "User.h"
#include "messages.h"

extern char* base_server_folder;

static constexpr int getMSGcode_Bool(bool flag)
{
    return (flag ? SERVER_MSG::ACTION_COMPLETED : SERVER_MSG::ERROR_OCCURED);
}

static constexpr int getMSGcode_syscall(int syscall)
{
    return (syscall ? SERVER_MSG::ERROR_OCCURED : SERVER_MSG::ACTION_COMPLETED);
}

uint64_t getSizeOfFile(const char* path)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define bswap64(y) (((uint64_t)ntohl(y)) << 32 | ntohl(y>>32))
#else
#define bswap64(y) (y)
#endif

    uint64_t l = (uint64_t)-1;
#ifdef WIN32
    HANDLE MF = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    DWORD High;
    if ( MF != INVALID_HANDLE_VALUE )
    {
        l = GetFileSize(MF, &High);
    }
#else
    struct stat st;
    if(stat(path, &st) >= 0)
        l = st.st_size;
#endif
    return bswap64(l);
#undef bswap64
}

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
    replaceSeperator(relDirP);
    strcat(result, (realativDirectory[0] == PATH_SEPERATOR_GOOD ? realativDirectory + 1 : realativDirectory));
    return (true);
}

int getContentDirectory(fsData* data)
{
    // data->data.path is used for storing the continue of path -> and it changes
    char *dirP, *resP;
    DIR* dir = NULL;
    dirP = data->data.path + strlen(data->data.path);
    if(*(dirP - 1) != PATH_SEPERATOR_GOOD)
        *(dirP++) = PATH_SEPERATOR_GOOD;

    char result[SERVER_BUFFER_SIZE + 5];
    resP = result; // a pointer to the last character

    struct dirent *ent;
    dir = opendir (data->data.path);
    if(!dir)
        return getMSGcode_Bool(false);
    int fileNameLen;

    struct __attribute__((packed)){
        uint64_t size;
        uint8_t flags;
    } fileStat;
    static constexpr unsigned MIN_REMAINING_LENGTH = sizeof(fileStat) + 3;
    static constexpr unsigned FLAG_ISDIR = 0x1;

    while ((ent = readdir (dir)))
    {
        if(ent->d_name[0] == '.' && (ent->d_name[1] == 0 || (ent->d_name[1] == '.' && ent->d_name[2] == 0))) // the path is not "." or ".."
            continue;
        if((fileNameLen = strlen(ent->d_name)) == 0)
            continue;

        if(resP - result + fileNameLen >= SERVER_BUFFER_SIZE - MIN_REMAINING_LENGTH)
        {
            data->user->sendData(SERVER_MSG::LS_DATA, result, resP - result);
            resP = result;
        }

        memcpy(dirP, ent->d_name, fileNameLen + 1);
        if(isDirectory(data->data.path))
        {
            fileStat.flags |= FLAG_ISDIR;
            fileStat.size = 0;
        }
        else
        {
            fileStat.flags = 0;
            fileStat.size = getSizeOfFile(data->data.path);
        }
        memcpy(resP, &fileStat, sizeof(fileStat));
        resP += sizeof(fileStat);
        memcpy(resP, ent->d_name, fileNameLen + 1);
        resP += fileNameLen + 1;
    }
    closedir (dir);
    if(resP != result)
        data->user->sendData(SERVER_MSG::LS_DATA, result, resP - result);
    return getMSGcode_Bool(true);
}

#ifndef WIN32
int symbolicLink(fsData* data)
{
    char srcT[FILENAME_MAX], srcR[FILENAME_MAX];
    strncpy(srcR, data->data.path2.src, FILENAME_MAX - 1);
    while(readlink(srcR, srcT, FILENAME_MAX - 1) > 0)
        strncpy(srcR, srcT, FILENAME_MAX - 1);
    return getMSGcode_syscall(symlink(srcR, data->data.path2.dst));
}
#endif

int createDirectory(fsData* data)
{
#ifdef WIN32
    return getMSGcode_Bool(CreateDirectoryA(data->data.path, NULL));
#else
    struct stat st;
    memset(&st, 0, sizeof(struct stat));
    return getMSGcode_syscall(stat(data->data.path, &st) && mkdir(data->data.path, 0755));
#endif
}

int moveFile(fsData* data)
{
    return getMSGcode_syscall(rename(data->data.path2.src, data->data.path2.dst));
}

int copyFile(fsData* data)
{
#ifdef WIN32
    return getMSGcode_Bool(CopyFileA(data->data.path2.src, data->data.path2.dst, 0));
#else
    int src = open(data->data.path2.src, O_RDONLY, 0);
    int dst = open(data->data.path2.dst, O_WRONLY | O_CREAT, 0644);
    struct stat stat_source;
    fstat(src, &stat_source);
    int res = getMSGcode_Bool(sendfile(dst, src, 0, stat_source.st_size) == stat_source.st_size);
    close(dst);
    close(src);
    return res;
#endif
}

int removeFile(fsData* data)
{
#ifdef WIN32
    return getMSGcode_Bool(DeleteFileA(data->data.path));
#else
    return getMSGcode_syscall(unlink(data->data.path));
#endif
}

int getFileStat(fsData* data)
{
    struct __attribute__((packed)){
        uint64_t size;
        uint8_t flags = 0;
    } buffer;
    static constexpr unsigned FLAG_ISDIR = 0x1;
    if(!isFileExists(data->data.path))
        return getMSGcode_Bool(false);
    if(isDirectory(data->data.path))
    {
        buffer.flags |= FLAG_ISDIR;
        buffer.size = 0;
    }
    else
    {
        buffer.size = getSizeOfFile(data->data.path);
    }
    data->user->sendData(SERVER_MSG::ACTION_COMPLETED, &buffer, sizeof(buffer));
    return (-1);
}

int getMD5OfFile(fsData* data)
{
    static constexpr unsigned READ_BUFFER = 512;

    int i;
    FILE* f;
    MD5_CTX ctx;
    uint8_t buffer[READ_BUFFER];
    uint8_t result[MD5_DIGEST_LENGTH];

    if(!(f = fopen(data->data.path, "rb")))
    {
        return getMSGcode_Bool(false);
    }

    MD5_Init(&ctx);
    while((i = fread(buffer, 1, 512, f)))
        MD5_Update(&ctx, buffer, i);
    MD5_Final(result, &ctx);

    data->user->sendData(SERVER_MSG::ACTION_COMPLETED, result, MD5_DIGEST_LENGTH);
    fclose(f);
    return -1;
}

#ifndef WIN32
static int _remove_file(const char *fpath, const struct stat*, int, struct FTW*)
{
    return remove(fpath);
}
#endif

int removeFolder(fsData* data)
{
#ifdef WIN32
    char command[REL_PATH_MAX + BASE_FOLDER_MAX + 9] = "rd /q /s ";
    const int baseCommandLen = 9;

    strcpy(command + baseCommandLen, data->data.path);
    return getMSGcode_syscall(system(command));
#else
    return getMSGcode_syscall(nftw(data->data.path, _remove_file, 64, FTW_DEPTH | FTW_PHYS));
#endif
}

int copyFolder(fsData* data)
{
#ifdef WIN32
    char command[2 * (REL_PATH_MAX + BASE_FOLDER_MAX) + 20] = "xcopy /E /H /Y /i ";
    const int baseCommandLen = 18;

    char* cP = command + baseCommandLen;
    strcpy(cP, data->data.path2.src);
    *(cP += strlen(cP)) = ' ';
    strcpy(cP + 1, data->data.path2.dst);

    return getMSGcode_syscall(system(command));
#else
    pid_t p;
    int status;
    switch((p = fork()))
    {
        case -1:
            return getMSGcode_Bool(false);
        case 0:
            close(STDOUT_FILENO);
            close(STDERR_FILENO);
            execlp("cp", "-rf", data->data.path2.src, data->data.path2.dst, NULL);
            break;
        default:
            waitpid(p, &status, 0);
            return getMSGcode_syscall(WEXITSTATUS(status));
            break;
    }
    return getMSGcode_Bool(false);
#endif
}

bool isFileExists(const char* path)
{
#ifdef WIN32
    return (GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES);
#else
    return (access(path, F_OK) == 0);
#endif
}
