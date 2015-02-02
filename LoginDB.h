#ifndef LOGINDB_H_
#define LOGINDB_H_

#include "defenitions.h"
#include "Login.h"

using namespace std;

/*
 * a class for holding the login database.
 * the database is held in the HEAP memory.
 */
class LoginDB {
private:
    Login* head;
    unsigned short count;

    void add(Login* next);
public:
    /*
     * create empty list
     */
    LoginDB()
    {
        this->count = 0;
        this->head = NULL;
    }

    /*
     * Load the list from the given file.
     * If error occurs on loading, the count variable will be empty.
     */
    LoginDB(const char* filePath)
    {
        this->count = 0;
        this->head = NULL;
        load(filePath);
    }

    void add(const char* username, const char* password, Login::LOGIN_ACCESS state);
    /*
     * Returns pointer to the Login object that matches the given login. If not found, returns NULL.
     */
    const Login* check(const char* username, const uint8_t passwordMD5[16]) const;
    bool save(const char* path) const;
    inline bool isEmpty() const
    {
        return (this->count == 0);
    }
    void print() const;
    void input();
    void load(const char* filePath);
    /*
     * removed the Login in place number (starts with 1)
     */
    bool remove(int number);
    ~LoginDB()
    {
        if(this->head)
            delete (this->head);
        this->head = nullptr;
    }
};

#endif /* LOGINDB_H_ */
