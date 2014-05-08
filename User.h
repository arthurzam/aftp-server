#ifndef USER_H_
#define USER_H_

#include <ctime>
#include <cstdio>
#ifdef WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include "defenitions.h"
#include "fileControl.h"
#include "server.h"
#include "FileTransfer.h"
#include "Login.h"

class Login;
class FileTransfer;

class User {
private:
    struct sockaddr_in _from;
    char _folderPath[FILENAME_MAX];
    time_t _lastUse;
    bool_t _timeout;
    bool_t _logedIn;
    bool_t _initialized;
    Login* _login;

    /*
     * changes the folder path using the given path (cdPath).
     * the starting position should be in destination, and the result would be there.
     * moves through every path between slash.
     * possible bad parts of path:
     *    two slash one after another
     *    move back (..) when already on root
     * return (TRUE) if everything is OK, and (FALSE) if error.
     */
    static bool_t parseChangeDir(char* cdPath, char* destination);
public:
    FileTransfer* fileTransfer;
    User();
    User(const struct sockaddr_in& from);
    ~User();

    void resetTime();

    /*
     * returns TRUE if user is to long not activated (timeout send + time moved).
     * if the user is inactive first time, then sends timeout message (900) to this user.
     */
    bool_t timeout();
    inline bool_t isLoged() const
    {
        return (this->_logedIn);
    }
    inline void logIn(Login* login)
    {
        this->_logedIn = TRUE;
        this->_login = login;
    }
    inline const char* folderPath() const
    {
        return (this->_folderPath);
    }

    /*
     * returns a new char array allocated in HEAP that contains the path of the given path by the current folder,
     */
    char* getRealFile(char* relativeFile, char* result) const;
    inline bool_t moveFolder(char* path)
    {
        return (User::parseChangeDir(path, this->_folderPath));
    }

    bool_t equals(const struct sockaddr_in& other) const;
    inline bool_t equals(const User& other) const
    {
        return (this->equals(other._from));
    }

    void print() const;

    inline int sendData(short msgCode, void* data, int datalen)
    {
        return (sendMessage(&this->_from, msgCode, data, datalen));
    }

    inline int sendData(short msgCode)
    {
        return (sendMessage(&this->_from, msgCode, NULL, 0));
    }
};

#endif
