#include <cstring>
#include "User.h"
#include "server.h"

User::User()
{
    this->_folderPath[0] = 0;
    this->_lastUse = 0;
    this->_logedIn = FALSE;
    this->_timeout = FALSE;
    this->_initialized = FALSE;
    this->fileTransfer = NULL;
}

User::User(const struct sockaddr_in& from)
{
    this->_folderPath[0] = 0;
    memcpy(&(this->_from), &from, sizeof(struct sockaddr_in));
    this->_lastUse = time(NULL);
    this->_logedIn = FALSE;
    this->_timeout = FALSE;
    this->_initialized = TRUE;
    this->fileTransfer = NULL;
}

User::~User()
{
    if(fileTransfer)
        delete fileTransfer;
    fileTransfer = NULL;
}

void User::resetTime()
{
    this->_lastUse = time(NULL);
    this->_timeout = FALSE;
}

bool_t User::equals(const struct sockaddr_in& other) const
{
    if(other.sin_port != this->_from.sin_port)
        return (FALSE);
#ifdef WIN32
    if(other.sin_addr.S_un.S_addr != this->_from.sin_addr.S_un.S_addr)
#else
    if(other.sin_addr.s_addr != this->_from.sin_addr.s_addr)
#endif
        return (FALSE);
    return (TRUE);
}

bool_t User::moveFolder(char* path)
{
    if(*path == 0)
        return (TRUE);
    int i, j;
    int last = -1;
    char newStr[FILENAME_MAX] = {PATH_SEPERATOR_GOOD, 0};

    // replace all \ into /
    for(i = 0; path[i] != 0; ++i)
    {
        if(path[i] == PATH_SEPERATOR_BAD)
            path[i] = PATH_SEPERATOR_GOOD;
        if(i > 0 && path[i] == PATH_SEPERATOR_GOOD && path[i - 1] == PATH_SEPERATOR_GOOD) // we have two slashes one by one
            return (FALSE);
    }
    if(path[i - 1] == PATH_SEPERATOR_GOOD) // we can use i-1 because path[i] after the for is the '/0' ending
        path[i - 1] = 0; // if the path finishes in divider, remove it

    if(*path == PATH_SEPERATOR_GOOD) // for example /path/folder => just replace the current
    {
        strcpy(this->_folderPath, path);
        return (TRUE);
    }

    if(this->_folderPath) // we have previous path
    {
        strcpy(newStr, this->_folderPath);
    }

_check_path:
    if(!*path) // path has ended
        goto _fin;
    if(*path == '.' && path[1] == '.')
    {
        if(newStr[1] == 0)
            return (FALSE); // trying to move back when we are already on root
        for(i = 0; newStr[i] != 0; ++i)
            if(newStr[i] == PATH_SEPERATOR_GOOD)
                last = i; // find the last occurrence of '/'
        newStr[last] = 0;
        if(path[2] == 0)
            goto _fin;
        path += 3;
        goto _check_path;
    }

    for(i = 0; path[i] != 0 && path[i] != '/'; ++i) ; // find the first occurrence of '/'
    last = path[i];
    path[i] = 0;
    j = strlen(newStr);
    newStr[j++] = PATH_SEPERATOR_GOOD;
    strcpy(newStr + j, path);
    path[i] = last;
    path += i + 1;
    if(last != 0)
        goto _check_path;

_fin:
    strcpy(this->_folderPath, newStr);
    return (TRUE);
}

bool_t User::isLoged() const
{
    return (this->_logedIn);
}

bool_t User::timeout()
{
    const int USER_TIME_MSG_SEND = 50;
    const int USER_TIME_REMOVE = 100;
    time_t now = time(NULL);
    double seconds = difftime(now, this->_lastUse);
    if(this->_timeout)
        return (seconds > USER_TIME_REMOVE);
    else if (seconds > USER_TIME_MSG_SEND)
    {
        this->sendData(900); // send timeout
        this->_timeout = TRUE;
    }
    return (FALSE);
}

void User::logIn()
{
    this->_logedIn = TRUE;
}

char* User::getRealFile(char* relativeFile, char* result)
{
    char backup[FILENAME_MAX];
    char* res = NULL;
    strcpy(backup, this->_folderPath);
    if(!this->moveFolder(relativeFile))
    {
        goto _exit;
    }
    res = getRealDirectory(this->_folderPath, result);
_exit:
    strcpy(this->_folderPath, backup);
    return (res);
}

void User::print() const
{
    if(this->_initialized)
    {
        printf("{<%s:%d>, %s}\n", inet_ntoa(this->_from.sin_addr), this->_from.sin_port, (this->_folderPath[0] ? this->_folderPath : "/"));

    }
}
