#ifndef LOGINDB_H_
#define LOGINDB_H_

#include <string.h>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include "defenitions.h"
#include "md5.h"

using namespace std;

#define USERNAME_MAX_LENGTH 32

typedef struct _login_t {
    char username[USERNAME_MAX_LENGTH];
    byte_t password[MD5_RESULT_LENGTH];
    struct _login_t* next;
} login_t;

/*
 * a class for holding the login database.
 * the database is held in the HEAP memory.
 */
class LoginDB {
private:
    login_t* head;
    unsigned short count;
public:
    /*
     * create empty list
     */
    LoginDB();
    /*
     * Load the list from the given file.
     * If error occurs on loading, the count variable will be empty.
     */
    LoginDB(const char* filePath);
    void add(const char* username, const char* password);
    bool_t check(const char* username, const byte_t passwordMD5[16]) const;
    bool_t save(const char* path) const;
    bool_t isEmpty() const;
    void print() const;
    void load(const char* filePath);
    ~LoginDB();
};

#endif /* LOGINDB_H_ */
