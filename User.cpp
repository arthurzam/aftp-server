#include "User.h"

User::User()
{
    this->_folderPath[0] = 0;
    this->_lastUse = 0;
    this->_logedIn = FALSE;
    this->_timeout = FALSE;
    this->_initialized = FALSE;
    this->fileTransfer = NULL;
    this->_login = NULL;
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
    this->_login = NULL;
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
    return (other.sin_port == this->_from.sin_port &&
#ifdef WIN32
        other.sin_addr.S_un.S_addr == this->_from.sin_addr.S_un.S_addr
#else
        other.sin_addr.s_addr == this->_from.sin_addr.s_addr
#endif
        );
}

bool_t User::parseChangeDir(char* path, char* dst)
{
    if(*path == 0)
        return (TRUE);
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
            return (FALSE); // trying to move back when we are already on root
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
    return (TRUE);
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

char* User::getRealFile(char* relativeFile, char* result) const
{
    char dst[FILENAME_MAX];
    char* res = NULL;
    strcpy(dst, this->_folderPath);
    if(User::parseChangeDir(relativeFile, dst))
    {
        res = getRealDirectory(dst, result);
    }
    return (res);
}

void User::print() const
{
    if(this->_initialized)
    {
        printf("{<%s:%d>, %s}\n", inet_ntoa(this->_from.sin_addr), this->_from.sin_port, (this->_folderPath[0] ? this->_folderPath : "/"));

    }
}
