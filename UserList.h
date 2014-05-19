#ifndef AFTP_SRC_USERLIST_H_
#define AFTP_SRC_USERLIST_H_

#include "User.h"
#include "defenitions.h"

#define USERS_IN_USERS_ARRAY 0x400 // =1024

class User;

class UserList {
private:
    typedef struct _listNode {
        User* arr[USERS_IN_USERS_ARRAY];
        bool isFull;
        struct _listNode* next;
    } ListNode;

    int userCount;
    int nodesCount;
    ListNode* head;
    mutable bool isSearching;

    static ListNode* createNewNode();
public:
    UserList();
    ~UserList();

    int removeUser(int index);
    int removeUser(const User* user);
    int addUser(const struct sockaddr_in& user);
    int findIndexOfUser(const User& user) const;
    User* findUser(const struct sockaddr_in& user) const;

    User* operator[](int index) const;

    inline int getUserCount() const
    {
        return (this->userCount);
    }

    void userControl();
    void print() const;
};
#endif
