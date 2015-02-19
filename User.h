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
    struct sockaddr_in _from;
    bool _timeout = false;
    bool _initialized = false;
    bool _isEncrypted = false;
    char _folderPath[REL_PATH_MAX] = {0};
    AES_KEY aesKeyDecrypt;
    AES_KEY aesKeyEncrypt;


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

    User() = delete;
    User(const struct sockaddr_in& from);
    ~User() {}

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
        //void* n = (-this->_isEncrypted) & (&this->aesKeyEncrypt);
        return (sendMessage(&this->_from, msgCode, data, datalen,
                (this->_isEncrypted ? &this->aesKeyEncrypt : nullptr)));
    }

    inline int sendData(uint16_t msgCode) const
    {
        return (this->sendData(msgCode, nullptr, 0));
    }

    inline void setAesKey(const uint8_t key[AES_KEY_LENGTH / 8])
    {
        AES_set_encrypt_key(key, AES_KEY_LENGTH, &this->aesKeyEncrypt);
        AES_set_decrypt_key(key, AES_KEY_LENGTH, &this->aesKeyDecrypt);
        this->_isEncrypted = true;
    }
    inline bool isEncryption() const
    {
        return this->_isEncrypted;
    }
    inline void stopEncryption()
    {
        this->_isEncrypted = false;
    }
    const AES_KEY* getDecryptKey() const
    {
        return &this->aesKeyDecrypt;
    }
};

#endif
