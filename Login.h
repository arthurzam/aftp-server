#ifndef LOGIN_H_
#define LOGIN_H_

#include <openssl/md5.h>
#include <cstdio>
#include "defenitions.h"

/*
 * A class for holding login data, the permissions per Login and etc'.
 */
class Login {
    public:
        static constexpr unsigned USERNAME_MAX_LENGTH = 32;
    private:
        typedef struct _folder {
            char folder[REL_PATH_MAX + 1];
            unsigned short folderLen;
            struct _folder* next;
        } folder;

        char username[USERNAME_MAX_LENGTH];
        uint8_t password[MD5_DIGEST_LENGTH];
        uint8_t state;
        folder* restrictedFolders;
        uint8_t restrictedFoldersCount;
        bool isInit;
        Login* _next;
    public:
        enum LOGIN_ACCESS {
            ADMIN = 0,
            LIMITED,
            ALL
        };
        Login(const char* username, const uint8_t* password, LOGIN_ACCESS state);
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
            return (this->state == LOGIN_ACCESS::ADMIN);
        }

        inline Login*& next()
        {
            return (this->_next);
        }

        inline const Login* next() const
        {
            return (this->_next);
        }
};

#endif /* LOGIN_H_ */
