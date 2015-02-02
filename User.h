#ifndef USER_H_
#define USER_H_

#include <ctime>
#ifdef WIN32
#include <windows.h>
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
    bool _timeout;
    bool _initialized;
    const Login* _login;

    /*
     * changes the folder path using the given path (cdPath).
     * the starting position should be in destination, and the result would be there.
     * moves through every path between slash.
     * possible bad parts of path:
     *    move back (..) when already on root
     * return (TRUE) if everything is OK, and (FALSE) if error.
     */
    static bool parseChangeDir(char* cdPath, char* destination);
public:
    FileTransfer* fileTransfer;
    User();
    User(const struct sockaddr_in& from);

    void resetTime()
    {
        this->_lastUse = time(NULL);
        this->_timeout = false;
    }

    /*
     * returns TRUE if user is to long not activated (timeout send + time moved).
     * if the user is inactive first time, then sends timeout message (900) to this user.
     */
    bool timeout();
    inline bool isLoged() const
    {
        return (this->_login != NULL);
    }
    inline void logIn(const Login* login)
    {
        this->_login = login;
    }
    inline const char* folderPath() const
    {
        return (this->_folderPath);
    }

    /*
     * puts the full path in result array
     * return FALSE if error in move directory or if a restricted folder, otherwise TRUE.
     */
    bool getRealFile(char* relativeFile, char* result) const;
    inline bool moveFolder(char* path)
    {
        return (User::parseChangeDir(path, this->_folderPath));
    }

    bool equals(const struct sockaddr_in& other) const;
    inline bool equals(const User& other) const
    {
        return (this->equals(other._from));
    }

    void print() const;

    inline int sendData(uint16_t msgCode, const void* data, int datalen) const
    {
        return (sendMessage(&this->_from, msgCode, data, datalen));
    }

    inline int sendData(uint16_t msgCode) const
    {
        return (sendMessage(&this->_from, msgCode, NULL, 0));
    }

    inline void cleanFileTransfer(FileTransfer* ptr = nullptr)
    {
        if(this->fileTransfer)
            delete this->fileTransfer;
        this->fileTransfer = ptr;
    }

    ~User()
    {
        cleanFileTransfer();
    }
};

#endif
