#ifndef AFTP_SRC_USERLIST_H_
#define AFTP_SRC_USERLIST_H_

#include "User.h"
#include "defenitions.h"
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#define USERS_IN_USERS_ARRAY 0x400 // =1024

class User;


class UserList {

private:
    struct _listNode {
        User* arr[USERS_IN_USERS_ARRAY];
        bool_t isFull;
        struct _listNode* next;
    };
    typedef struct _listNode ListNode;

    int userCount;
    int nodesCount;
    ListNode* head;
    ListNode* createNewNode();
    mutable bool_t isSearching;
public:
    UserList();
    ~UserList();

    int removeUser(int index);
    int removeUser(const User* user);
    int addUser(const struct sockaddr_in&user);
    int findIndexOfUser(const User& user) const;
    User* findUser(const struct sockaddr_in& user) const;

    User* operator[](int index) const;
    inline int operator+=(const struct sockaddr_in&user)
    {
        return (this->addUser(user));
    }

    int getUserCount() const;

    void userControl();
    void print() const;
};
#endif
