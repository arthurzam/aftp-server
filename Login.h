#ifndef LOGIN_H_
#define LOGIN_H_

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <openssl/md5.h>
#include "defenitions.h"

#define USERNAME_MAX_LENGTH 32

/*
 * A class for holding login data, the permissions per Login and etc'.
 */
class Login {
    typedef struct _folder {
        char folder[FILENAME_MAX + 1];
        unsigned short folderLen;
        struct _folder* next;
    } folder;
private:
    char username[USERNAME_MAX_LENGTH];
    uint8_t password[MD5_DIGEST_LENGTH];
    uint8_t state;
    folder* restrictedFolders;
    uint8_t restrictedFoldersCount;
    bool isInit;
    Login* _next;
public:
    enum LOGIN_ACCESS {
        LOGIN_ACCESS_ADMIN = 0,
        LOGIN_ACCESS_LIMITED,
        LOGIN_ACCESS_ALL
    };
    Login(const char* username, const uint8_t* password, LOGIN_ACCESS state);
    Login(const Login& other);
    Login(FILE* srcFile);
    ~Login();

    bool check(const char* username, const uint8_t* password) const;

    /*
     * the folder path should be the relative one!
     */
    void addRestrictedFolder(const char* folder);
    bool isRestrictedFolder(const char* path) const;

    bool save(FILE* dstFile) const;
    void print() const;

    inline bool isLoaded() const
    {
        return (this->isInit);
    }
    inline bool isAdmin() const
    {
        return (this->state == LOGIN_ACCESS_ADMIN);
    }

    inline Login*& next()
    {
        return (this->_next);
    }
};

#endif /* LOGIN_H_ */
