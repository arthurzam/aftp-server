#ifndef USER_H_
#define USER_H_

#include <ctime>
#ifdef WIN32
#include <windows.h>
#else
#include <arpa/inet.h>
#endif

#include "defenitions.h"
#include "server.h"
#include "FileTransfer.h"
#include "Login.h"

class User {
private:
    time_t _lastUse = 0;
    const Login* _login = nullptr;
    bool _timeout = false;
    bool _initialized = false;
    struct sockaddr_in _from = {0, 0, {0}, {0}};
    char _folderPath[FILENAME_MAX] = {0};

    /**
     * @brief changes the folder path using the given path
     *
     * @param cdPath move path to be parsed and affected onto @c destination
     * @param destination starting point path and output path
     * @return returns true if seceded, otherwise false.
     */
    static bool parseChangeDir(char* cdPath, char* destination);
public:
    FileTransfer fileTransfer;

    User() {}
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

    ~User() {}
};

#endif
