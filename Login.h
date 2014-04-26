#ifndef LOGIN_H_
#define LOGIN_H_

#include <cstdio>
#include <cstring>
#include "defenitions.h"
#include "md5.h"

#define USERNAME_MAX_LENGTH 32

enum LOGIN_ACCESS {
    LOGIN_ACCESS_ADMIN = 0,
    LOGIN_ACCESS_READ_ONLY,
    LOGIN_ACCESS_LIMITED,
    LOGIN_ACCESS_LIMITED_READ_ONLY,
    LOGIN_ACCESS_ALL
};

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
    byte_t password[MD5_RESULT_LENGTH];
    byte_t state;
    folder* restrictedFolders;
    byte_t restrictedFoldersCount;
    bool_t isInit;
    Login* _next;
public:
    Login(const char* username, const byte_t* password, byte_t state);
    Login(const Login& other);
    Login(FILE* srcFile);
    ~Login();

    bool_t check(const char* username, const byte_t* password) const;

    /*
     * the folder path should be the relative one!
     */
    void addRestrictedFolder(const char* folder);
    bool_t isRestrictedFolder(const char* path) const;

    bool_t save(FILE* dstFile) const;
    void print() const;

    bool_t isLoaded() const
    {
        return (this->isInit);
    }

    Login*& next()
    {
        return (this->_next);
    }
};

#endif /* LOGIN_H_ */
