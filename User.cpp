#include <cstring>

#include "User.h"
#include "fileControl.h"
#include "messages.h"

User::User(const struct sockaddr_in& from)
{
    memcpy(&(this->_from), &from, sizeof(struct sockaddr_in));
    this->_lastUse = time(NULL);
    this->_initialized = true;
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
    if(path[0] == '\0')
        return (true);
    int i;
    char* temp = path - 1;
    char newStr[BASE_FOLDER_MAX + REL_PATH_MAX] = {'\0', };
    replaceSeperator(temp);
    if(path[i = strlen(path) - 1] == PATH_SEPERATOR_GOOD) // we can use i-1 because path[i] after the for is the '/0' ending
        path[i] = '\0'; // if the path finishes in divider, remove it

    if(path[0] == PATH_SEPERATOR_GOOD) // for example /path/folder => remove previous
        path++;
    else if(dst[0]) // we have previous path
        strncpy(newStr, dst, sizeof(newStr) - 1);

    while(path[0] != '\0')
    {
        if(path[0] == '.')
        {
            if((path[1] == '.') & ((path[2] == PATH_SEPERATOR_GOOD) | (path[2] == '\0'))) // ../
            {
                if(newStr[1] != '\0') // if root -> do nothing
                {
                    // find the last occurrence of '/' and set it as '\0'
                    if(!(temp = strrchr(newStr, PATH_SEPERATOR_GOOD)))
                    {
                        newStr[0] = '\0';
                        break;
                    }
                    temp[0] = '\0';
                }
                if(path[2] == '\0')
                    break;
                path += 3;
                continue;
            }
            else if((path[1] == PATH_SEPERATOR_GOOD) | (path[1] == '\0')) // just . -> current dir
            {
                if(path[1] == '\0')
                    break;
                path += 2;
                continue;
            }
        }
        i = (((temp = strchr(path, PATH_SEPERATOR_GOOD))) ? temp - path : strlen(path)); // find the first occurrence of '/', or whole length if no '/'
        if(i != 0)
        {
            *(temp = newStr + strlen(newStr)) = PATH_SEPERATOR_GOOD;
            *((char*)memcpy(temp + 1, path, i) + i) = '\0';
        }
        path += i + 1;
        if(path[-1] == '\0')
            break;
    }

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
    char dst[FILENAME_MAX + 8];
    strcpy(dst, this->_folderPath);
    char* tempPtr;
    if(User::parseChangeDir(relativeFile, dst))
    {
        *(tempPtr = dst + strlen(dst)) = PATH_SEPERATOR_GOOD; // we add just in case the / at end for isRestricted()
        *(tempPtr + 1) = '\0';
        if (this->_login->isRestrictedFolder(dst))
            return (false);
        *tempPtr = '\0';
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
