#ifndef USER_H_
#define USER_H_

#include <string>
#include <ctime>
#include <cstdio>
#include "defenitions.h"
#include "fileControl.h"
#include "server.h"
#include "FileTransfer.h"

#ifdef WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif
using namespace std;

#define USER_TIME_MSG_SEND 50
#define USER_TIME_REMOVE 100

class FileTransfer;

class User {
private:
    struct sockaddr_in _from;
    char _folderPath[FILENAME_MAX];
    time_t _lastUse;
    bool_t _timeout;
    bool_t _logedIn;
    bool_t _initialized;

    void copyFrom(const struct sockaddr_in* from);
public:
    FileTransfer* fileTransfer; // TODO: convert to private member
    User();
    User(const struct sockaddr_in* from);
    User(const User& other);
    ~User();

    void resetTime();
    void reset(const struct sockaddr_in* from);

    /*
     * returns TRUE if user is to long not activated (timeout send + time moved).
     * if the user is inactive first time, then sends timeout message (900) to this user.
     */
    bool_t timeout();
    bool_t isLoged() const;
    void logIn();
    const char* folderPath() const;

    /*
     * returns a new char array allocated in HEAP that contains the path of the given path by the current folder,
     * but still don't contain the real path on hard disk (without SERVER_BASE_FOLDER)
     */
    char* getRealFile(char* relativeFile, char* result);

    /*
     * changes the folder path using the given path.
     * moves through every path between slash.
     * Therefore if there is suddenly a bad part of path, all the parts until the bad part.
     * possible bad parts of path:
     *    two slash one after another
     *    move back (..) when already on root
     * return (TRUE) if everything is OK, and (FALSE) if error.
     */
    bool_t moveFolder(char* path);

    bool_t equals(const User& other) const;
    inline bool_t operator==(const User& other) const
    {
        return (this->equals(other));
    }
    inline bool_t operator!=(const User& other) const
    {
        return (this->equals(other));
    }

    void print() const;

    inline int sendData(short msgCode, char* data, int datalen)
    {
        return (sendMessage(&this->_from, msgCode, data, datalen));
    }

    inline int sendData(short msgCode)
    {
        return (sendMessage(&this->_from, msgCode, NULL, 0));
    }
};

#endif
