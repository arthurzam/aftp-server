#include <cstdio>
#include <cstring>

#include "User.h"
#include "fileControl.h"
#include "messages.h"

User::User()
{
    this->_folderPath[0] = 0;
    this->_lastUse = 0;
    this->_timeout = false;
    this->_initialized = false;
    this->_login = nullptr;
}

User::User(const struct sockaddr_in& from)
{
    this->_folderPath[0] = 0;
    memcpy(&(this->_from), &from, sizeof(struct sockaddr_in));
    this->_lastUse = time(NULL);
    this->_timeout = false;
    this->_initialized = false;
    this->_login = nullptr;
}

bool User::equals(const struct sockaddr_in& other) const
{
    return (other.sin_port == this->_from.sin_port &&
#ifdef WIN32
            other.sin_addr.S_un.S_addr == this->_from.sin_addr.S_un.S_addr
#else
            other.sin_addr.s_addr == this->_from.sin_addr.s_addr
#endif
           );
}

bool User::parseChangeDir(char* path, char* dst)
{
    if(*path == 0)
        return (true);
    int i;
    char* temp = path - 1;
    char newStr[FILENAME_MAX];
    newStr[0] = 0;
    while((temp = strchr(temp + 1, PATH_SEPERATOR_BAD))) // replace every PATH_SEPERATOR_BAD into PATH_SEPERATOR_GOOD
        *temp = PATH_SEPERATOR_GOOD;
    if(path[i = strlen(path) - 1] == PATH_SEPERATOR_GOOD) // we can use i-1 because path[i] after the for is the '/0' ending
        path[i] = 0; // if the path finishes in divider, remove it

    if(*path == PATH_SEPERATOR_GOOD) // for example /path/folder => remove previous
        path++;
    else if(dst[0]) // we have previous path
        strcpy(newStr, dst);

_check_path:
    if(!*path) // path has ended
        goto _fin;
    if(*path == '.' && path[1] == '.')
    {
        if(newStr[1] == 0)
            return (false); // trying to move back when we are already on root
        temp = strrchr(newStr, PATH_SEPERATOR_GOOD); // find the last occurrence of '/' and set it as '\0'
        if(!temp)
        {
            newStr[0] = 0;
            goto _fin;
        }
        *temp = 0;
        if(path[2] == 0)
            goto _fin;
        path += 3;
        goto _check_path;
    }
    i = (((temp = strchr(path, PATH_SEPERATOR_GOOD))) ? temp - path : strlen(path)); // find the first occurrence of '/', or whole length if no '/'
    if(i != 0)
    {
        *(temp = newStr + strlen(newStr)) = PATH_SEPERATOR_GOOD;
        *((char *)memcpy(temp + 1, path, i) + i) = 0;
    }
    path += i + 1;
    if(path[-1])
        goto _check_path;

_fin:
    strcpy(dst, newStr);
    return (true);
}

bool User::timeout()
{
    static constexpr unsigned USER_TIME_MSG_SEND = 50;
    static constexpr unsigned USER_TIME_REMOVE   = 100;

    time_t now = time(NULL);
    double seconds = difftime(now, this->_lastUse);
    if(this->_timeout)
        return (seconds > USER_TIME_REMOVE);
    else if (seconds > USER_TIME_MSG_SEND)
    {
        this->sendData(SERVER_MSG::TIMEOUT); // send timeout
        this->_timeout = true;
    }
    return (false);
}

bool User::getRealFile(char* relativeFile, char* result) const
{
    char dst[FILENAME_MAX];
    strcpy(dst, this->_folderPath);
    char* tempPtr;
    if(User::parseChangeDir(relativeFile, dst))
    {
        *(tempPtr = dst + strlen(dst)) = PATH_SEPERATOR_GOOD; // we add just in case the / at end for isRestricted()
        *(tempPtr + 1) = 0;
        if (this->_login->isRestrictedFolder(dst))
            return (false);
        *tempPtr = 0;
        return (getRealDirectory(dst, result));
    }
    return (true);
}

void User::print() const
{
    if(this->_initialized)
    {
        printf("{<%s:%d>, %s}\n", inet_ntoa(this->_from.sin_addr), this->_from.sin_port, (this->_folderPath[0] ? this->_folderPath : "/"));
    }
}
